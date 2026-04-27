# =============================================================================
# buildwasm.ps1 — Build Chub Game Hub for WebAssembly
#
# Usage:
#   .\buildwasm.ps1                 — clean, compile, and run
#   .\buildwasm.ps1 -clean          — clean build artifacts
#   .\buildwasm.ps1 -compile        — compile only
#   .\buildwasm.ps1 -compileAndRun  — compile and open in browser
#   .\buildwasm.ps1 -run            — open in browser (must be compiled)
#
# Requirements:
#   - Emscripten SDK (emcc in PATH)
#   - Python 3 (for local HTTP server)
#
# Output:
#   web/chub.html, chub.js, chub.wasm, chub.data
# =============================================================================

param(
    [switch]$clean,
    [switch]$cleanAndCompile,
    [switch]$compile,
    [switch]$compileAndRun,
    [switch]$run
)

$ProjectName = "chub"
$BuildDir = "web"
$ServerPort = 8080

$all = $false
if (-not ($clean -or $cleanAndCompile -or $compile -or $compileAndRun -or $run)) {
    $all = $true
}

# ── Clean ──
if ($clean -or $cleanAndCompile -or $all) {
    Write-Host "🧹 Cleaning..." -ForegroundColor Yellow
    if (Test-Path $BuildDir) {
        Remove-Item $BuildDir -Recurse -Force
    }
    if ($clean) { exit 0 }
}

# ── Compile ──
if ($compile -or $cleanAndCompile -or $compileAndRun -or $all) {
    Write-Host "🔨 Compiling for WebAssembly..." -ForegroundColor Cyan

    # Ensure output directory exists
    if (-not (Test-Path $BuildDir)) {
        New-Item -Path $BuildDir -ItemType Directory | Out-Null
    }

    # Ensure assets directories exist
    @("assets\shared", "assets\pong", "assets\crappybird", "assets\snake", "assets\tectris") | ForEach-Object {
        if (-not (Test-Path $_)) {
            New-Item -Path $_ -ItemType Directory -Force | Out-Null
        }
        if (-not (Test-Path "$_\.gitkeep") -and (Get-ChildItem $_ -ErrorAction SilentlyContinue).Count -eq 0) {
            New-Item -Path "$_\.gitkeep" -ItemType File -Force | Out-Null
        }
    }

    # ── Source files ──
    # Core
    $Sources = @(
        "./core/main.c",
        "./core/hub.c",
        "./core/registry.c"
    )

    # Games
    $Sources += @(
        "./games/pong/pong.c",
        "./games/crappybird/crappybird.c",
        "./games/snake/snake.c",
        "./games/tectris/tectris.c"
    )

    # To add a new game:
    #   1. Add "./games/yourgame/yourgame.c" to $Sources above
    #   2. Make sure it follows the Chub game module pattern

    $SourceArgs = $Sources -join " "

    # Build command
    $EmccCmd = "emcc $SourceArgs " +
        "-o ./$BuildDir/$ProjectName.html " +
        "-Wall " +
        "-std=c99 " +
        "-D_DEFAULT_SOURCE " +
        "-Wno-missing-braces " +
        "-Os " +
        "-I ./core/include " +
        "-I ./core/include/raylib " +
        "-L ./lib/wasm/ " +
        "-s USE_GLFW=3 " +
        "-s ASYNCIFY " +
        "-s INITIAL_MEMORY=67108864 " +
        "-s ALLOW_MEMORY_GROWTH=1 " +
        "-s FORCE_FILESYSTEM=1 " +
        "--preload-file ./assets@/assets " +
        "--shell-file ./platform/web/shell.html " +
        "./lib/wasm/libraylib.a " +
        "-DPLATFORM_WEB " +
        "-s 'EXPORTED_FUNCTIONS=[""_free"",""_malloc"",""_main""]' " +
        "-s EXPORTED_RUNTIME_METHODS=ccall"

    Write-Host "  Sources: $($Sources.Count) files" -ForegroundColor DarkGray
    Invoke-Expression $EmccCmd

    if ($LASTEXITCODE -eq 0) {
        Write-Host "✅ Build successful! Output in $BuildDir/" -ForegroundColor Green
        Write-Host "   Files:" -ForegroundColor DarkGray
        Get-ChildItem $BuildDir | ForEach-Object {
            $size = if ($_.Length -gt 1MB) { "{0:N1} MB" -f ($_.Length / 1MB) }
                    elseif ($_.Length -gt 1KB) { "{0:N0} KB" -f ($_.Length / 1KB) }
                    else { "{0} B" -f $_.Length }
            Write-Host "   - $($_.Name) ($size)" -ForegroundColor DarkGray
        }
    } else {
        Write-Host "❌ Build failed!" -ForegroundColor Red
        exit 1
    }
}

# ── Run ──
if ($run -or $compileAndRun -or $all) {
    $HtmlFile = "$BuildDir/$ProjectName.html"

    if (-not (Test-Path $HtmlFile)) {
        Write-Host "❌ $HtmlFile not found! Compile first." -ForegroundColor Red
        exit 1
    }

    Write-Host "🌐 Starting local server on http://localhost:$ServerPort/$ProjectName.html" -ForegroundColor Green
    Write-Host "   Press Ctrl+C to stop" -ForegroundColor DarkGray

    # Open browser
    Start-Process "http://localhost:$ServerPort/$ProjectName.html"

    # Start Python HTTP server
    Set-Location $BuildDir
    python -m http.server $ServerPort
}
