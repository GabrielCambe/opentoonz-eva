# aiassist — resumo de mudancas e superficie de divergencia

**Atualizado:** 2026-08-29
**Fase:** p3 (fatia vertical do painel), aguardando build do usuario.

---

## 1. Arquivos novos — conflito impossivel com o upstream

| Arquivo | Linhas | Papel |
| --- | ---: | --- |
| `toonz/sources/aiassist/comfyclient.h/.cpp` | 268 | Cliente HTTP: POST /prompt, polling /history, GET /view. |
| `toonz/sources/aiassist/graphtemplate.h/.cpp` | 166 | Carrega, valida e aplica patch em templates por `_meta.title`. |
| `toonz/sources/aiassist/aiassistcontroller.h/.cpp` | 331 | Painel Qt, captura do frame, preview e insercao como level. |
| `toonz/sources/aiassist/graphs/*.json` | 2 arquivos | Templates SD 1.5 img2img e txt2img. |
| `tools/aiassist_oracle.py` | 150 | Implementacao de referencia do protocolo. |

## 2. Superficie de divergencia — arquivos mantidos pelo upstream

Sete pontos, oito linhas de codigo. Tudo em listas de itens semelhantes, onde um
conflito de merge se resolve mantendo as duas linhas.

| Arquivo | Insercao |
| --- | --- |
| `toonz/menubarcommandids.h` | `#define MI_OpenAIAssistPanel …` |
| `toonz/tpanels.cpp` | `#include "aiassistcontroller.h"` |
| `toonz/tpanels.cpp` | `AIAssistPanelFactory` + `openAIAssistPanelCommand` |
| `toonz/mainwindow.cpp` | `createMenuWindowsAction(MI_OpenAIAssistPanel, …)` |
| `toonz/menubar.cpp` | `addMenuItem(windowsMenu, MI_OpenAIAssistPanel)` |
| `toonz/CMakeLists.txt` | 3 headers + 3 sources nas listas existentes |
| `toonz/CMakeLists.txt` | `../../sources/aiassist` no `include_directories` |

**Correcao ao plano original:** nao existe `add_subdirectory(aiassist)`. O modulo
`stopmotion/` nao e um alvo CMake proprio — seus arquivos entram diretamente nas
listas de `toonz/CMakeLists.txt` (headers ~l.146, sources ~l.417). O `aiassist/`
segue o mesmo padrao. Duas linhas a mais de divergencia do que o previsto, mesma
natureza trivial.

**Correcao descoberta na primeira build (erro C1083).** Listar os arquivos nas
listas de HEADERS/SOURCES faz o CMake compila-los, mas nao coloca a pasta no
caminho de include. `tpanels.cpp` so resolve `"stopmotioncontroller.h"` porque
`../../sources/stopmotion` esta no `include_directories()` (~l.499). Foi preciso
adicionar `../../sources/aiassist` na mesma lista. Licao geral: em CMake,
*compilar um arquivo* e *poder incluir o header dele* sao duas configuracoes
independentes.

## 3. Decisoes de implementacao

- **URL em `QSettings`, nao em `Preferences`.** Usar a classe `Preferences` do
  OpenToonz adicionaria mais dois arquivos do upstream a superficie de divergencia.
  `QSettings` custa zero e resolve o mesmo. Migracao fica para a fase 4, se valer.
- **Insercao via `IoCmd::loadResources`.** O PNG e gravado em `+drawings/` da cena
  e carregado pelo caminho normal do OpenToonz, em vez de construir `TXshSimpleLevel`
  a mao. Herda undo, level set e politica de import sem reimplementar nada.
- **`AIASSIST_GRAPHS`.** Variavel de ambiente opcional apontando para a pasta de
  templates. Sem ela, o painel procura em `applicationDirPath()/aiassist/graphs`.
  Evita depender de uma etapa de deploy durante o desenvolvimento.
- **Referencia = frame corrente do level corrente.** `getCurrentLevel()->getSimpleLevel()`
  + `getFrame(fid, false)` -> `TRasterImageP` -> `rasterToQImage` -> PNG -> base64.
  Levels vetoriais nao sao suportados nesta fatia e produzem mensagem explicita.

## 4. Verificacao ainda pendente

Nao foi compilado. O agente que escreveu o codigo nao tem o toolchain MSVC nem
alcanca o `127.0.0.1` da maquina do usuario. O que foi verificado estaticamente:

- Todos os dez headers incluidos existem nos caminhos usados.
- Os dois templates passam na validacao de titulos e so referenciam modelos
  presentes no servidor.
- O contrato de patch foi executado ponta a ponta contra o ComfyUI vivo, pelo
  navegador, produzindo PNG nos dois templates.

Passos `p3-s0` e `p3-s6` (assignee=user) fecham a lacuna.
