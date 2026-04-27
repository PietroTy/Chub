# CHUB — Game Hub 🎮

Hub modular de jogos feito com **Raylib** e **C**, compilado para **WebAssembly** para rodar no navegador.

## Arquitetura

```
core/       → Engine: menu, state machine, loop, frame overlay
games/      → Módulos de jogos (plugáveis)
assets/     → Assets compartilhados e por jogo
lib/        → raylib (nativa + wasm)
platform/   → HTML shell para Emscripten
web/        → Saída WASM (deploy)
```

### Sistema de Resolução Dupla

- **Canvas (browser)**: Adapta-se ao viewport (960×720 padrão)
- **Jogo (interno)**: Cada jogo define sua própria resolução
- **Bordas**: Mostram nome do projeto, controles, créditos

## Jogos Disponíveis

| Jogo | Resolução | Orientação | Descrição |
|------|-----------|:---:|-----------|
| Pong | 640×480 | Horizontal | Pong clássico — Player vs CPU |
| Crappybird | 600×600 | Quadrado | Flappy bird clone com skins desbloqueáveis |
| snaCke | 600×600 | Quadrado | Snake com maçã, uva e laranja |
| teCtris | 300×600 | Vertical | Tetris com 8 peças (incluindo Q rara) |

## Requisitos

| Ferramenta | Para quê |
|-----------|----------|
| GCC / Make | Compilar C (Linux/macOS) ou MinGW (Windows) |
| [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html) | Compilar para WASM |
| [Git](https://git-scm.com/) | Controle de versão |
| [Python 3](https://www.python.org/) | Servidor HTTP local |

## Build Nativo (Desktop)

```bash
make              # compilar e rodar
make compile      # só compilar
make run          # só rodar
make clean        # limpar build
```

## Build Web (WASM)

### Linux / macOS (Bash)
```bash
# Ativar Emscripten (se necessário)
source ~/emsdk/emsdk_env.sh

./buildwasm.sh               # limpar, compilar e servir
./buildwasm.sh -compile      # só compilar
./buildwasm.sh -run          # só servir (precisa compilar antes)
```

### Windows (PowerShell)
```powershell
.\buildwasm.ps1               # limpar, compilar e servir
.\buildwasm.ps1 -compile      # só compilar
.\buildwasm.ps1 -run           # só servir (precisa compilar antes)
```

Acesse: `http://localhost:8080/chub.html`

## Deploy (GitHub Pages)

Para colocar o jogo online no GitHub Pages:

1. **Gere o build**:
   ```bash
   source ~/emsdk/emsdk_env.sh
   ./buildwasm.sh -compile
   ```

2. **Crie/Atualize o branch `gh-pages`**:
   O comando abaixo envia apenas a pasta `web/` para o branch de deploy:
   ```bash
   git add web -f
   git commit -m "Deploy: Atualizando versão web"
   git subtree push --prefix web origin gh-pages
   ```

3. **Configure no GitHub**:
   - Vá em **Settings** > **Pages** no seu repositório.
   - Em **Build and deployment**, selecione o branch `gh-pages` e a pasta `/ (root)`.
   - Salve e aguarde alguns minutos.
4. GitHub Actions publica automaticamente

## Adicionando um Novo Jogo

1. Crie `games/seujogo/seujogo.c` e `seujogo.h`
2. Implemente os 4 callbacks: `init`, `update`, `draw`, `unload`
3. Defina `Game SEUJOGO_GAME = { ... }` com nome, descrição e resolução
4. Em `core/include/registry.h`, adicione `extern Game SEUJOGO_GAME;`
5. Em `core/registry.c`, adicione `&SEUJOGO_GAME` ao array
6. Em `buildwasm.ps1`, adicione o `.c` na lista de sources
7. Recompile

### Regras do Módulo

- **Sem** `main()`, `InitWindow`, `CloseWindow`, `InitAudioDevice`
- **Sem** `while` loop — hub controla o ciclo
- **Sem** `BeginDrawing`/`EndDrawing` no draw — hub usa RenderTexture
- Estado como `static` — isola do resto
- Assets em `assets/<game>/`
- Incluir `chub_colors.h` se usar cores custom

## Licença

Projeto pessoal. Raylib é licenciado sob zlib/libpng.
Jogos originais por PietroTy (2024).
