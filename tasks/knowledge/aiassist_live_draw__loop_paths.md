---
name: aiassist_live_draw__loop_paths
description: O que Insert as Level faz ate o level set, quais sinais emite e por que o risco real nao e o laco por sinal, e sim a coluna corrente mudar para o PNG gerado.
sources: task aiassist-live-draw, fase p1, passo p1-s2
---

# Caminho de `Insert as Level` e o risco de realimentacao

**Criado:** 2026-09-03 | **Task:** `tasks/aiassist_live_draw.xml` | **Fase:** p1 (p1-s2)

## 1. Por que este mapeamento existe

I2 do Intent Brief diz que nenhum caminho pode fazer o painel disparar a si mesmo. O risco
declarado era `onInsertAsLevel()`: ele grava um PNG e manda o OpenToonz carregar, e carregar
altera o level set. A pergunta do p1-s2 e concreta: **isso emite o sinal que vai virar o
gatilho, e o nivel inserido vira o nivel corrente?**

## 2. O caminho, passo a passo

Origem: `toonz/sources/aiassist/aiassistcontroller.cpp:655` (`onInsertAsLevel`).

1. Escolhe um nome livre `aiassist_NNNN.png` dentro de `+drawings` e grava o QImage
   (`aiassistcontroller.cpp:666-684`).
2. Monta `IoCmd::LoadResourceArguments args(fp)` e chama `IoCmd::loadResources(args)`
   (`:686-687`). **Nao passa row nem col**, entao ambos ficam `-1`.
3. `IoCmd::loadResources` - `toonz/sources/toonz/iocommand.cpp:2465` - resolve os `-1` para a
   posicao corrente: `if (row0 == -1) row0 = app->getCurrentFrame()->getFrameIndex();` e
   `if (col0 == -1) col0 = app->getCurrentColumn()->getColumnIndex();` (`:2489-2491`).
4. Para um arquivo de nivel, chama o `loadResource` de arquivo (`iocommand.cpp:1051`), que
   antes de expor chama `beforeCellsInsert(...)` (`:1021-1023`) e depois
   `scene->getXsheet()->exposeLevel(row0, col0, ...)` (`:1025`).
5. `beforeCellsInsert` - `iocommand.cpp:342-362` - e o ponto que importa. Se as celulas de
   destino ja estao ocupadas, ou se o tipo de coluna nao bate, ele:
   - incrementa `col` (`:354`),
   - **chama `TApp::instance()->getCurrentColumn()->setColumnIndex(col)`** (`:355`),
   - insere a coluna nova (`:357`).
6. No fim, `loadResources` ainda faz `app->getCurrentFrame()->setFrameIndex(row0)` quando algo
   foi carregado (`iocommand.cpp:2718`).
7. Ao sair do escopo, `LoadScopedBlock::~ScopedBlock()` - `iocommand.cpp:2437-2449` - emite
   `notifyXsheetChanged()`, `notifyCastChange()`, marca a cena suja e chama
   `app->getCurrentTool()->onImageChanged(...)`.

## 3. Resposta a pergunta do plano

### 3.1 `Insert as Level` NAO emite `xshLevelChanged`

Varredura do corpo inteiro de `loadResources` (`iocommand.cpp:2465-2729`): nenhuma chamada a
`notifyLevelChange`, `getCurrentLevel()` ou `setLevel`. O destrutor do scoped block tambem nao
(`:2437-2449`). E o unico emissor possivel do sinal e `notifyLevelChange()`
(`txshlevelhandle.h:40`).

Tambem nao ha caminho indireto: `ToolHandle::onImageChanged` -
`toonz/sources/tnztools/toolhandle.cpp:133-166` - so troca o tipo de ferramenta e chama
`m_tool->onImageChanged()`, sem notificar o level handle. E em `TApp` nada converte
`xsheetChanged` ou `castChange` em `notifyLevelChange` (`toonz/sources/toonz/tapp.cpp:140-158`;
as unicas chamadas a `notifyLevelChange` fora das ferramentas e comandos de edicao estao em
`toonzlib/studiopalettecmd.cpp:357` e `:515`, que sao paleta).

**Consequencia para A3: o laco por sinal, do jeito que foi temido no Intent Brief, nao existe
no codigo de hoje.** A guarda contra ele e defesa em profundidade, nao correcao de um defeito
observado.

### 3.2 O nivel inserido VIRA, na pratica, o alvo corrente

Isso e o achado que muda a decisao. Pelo passo 5, quando a celula de destino esta ocupada - o
caso normal, porque o usuario acabou de desenhar ali - `beforeCellsInsert` insere uma coluna
nova e **move a coluna corrente para ela** (`iocommand.cpp:355`). O frame corrente tambem e
reposicionado (`:2718`).

Agora cruze isso com de onde sai a referencia. `AIAssistController::resolveCurrentCell`
(`aiassistcontroller.cpp:429-458`) resolve, no modo SceneFrame - o modo de quem desenha no
ComboViewer sobre uma coluna -, para `xsh->getCell(frameHandle->getFrame(), col)` com
`col = app->getCurrentColumn()->getColumnIndex()` (`:446-455`). E
`currentFrameAsPngBase64()` (`:490`) usa exatamente essa celula.

Ou seja: **depois de `Insert as Level`, a celula que alimenta a referencia e a celula do PNG
gerado.** A proxima geracao - disparada por qualquer gatilho, inclusive o de texto que ja
existe hoje - usaria a saida da IA como sua propria entrada.

Nao e um laco automatico: nada dispara sozinho. Mas e realimentacao de conteudo assim que o
usuario der o proximo gatilho, e degrada em geracoes sucessivas.

### 3.3 Sinais que a insercao emite de fato

| Sinal | Onde | O painel escuta hoje? |
| --- | --- | --- |
| `xsheetChanged` | `iocommand.cpp:2441` | nao |
| `castChange` | `iocommand.cpp:2444` | nao |
| `columnIndexSwitched` | via `setColumnIndex`, `iocommand.cpp:355` | **sim** - `aiassistcontroller.cpp:177`, so chama `refresh` (texto de status) |
| `frameSwitched` | via `setFrameIndex`, `iocommand.cpp:2718` | **sim** - `aiassistcontroller.cpp:176`, so chama `refresh` |
| `xshLevelChanged` | nenhum | - (e o gatilho novo) |

As duas conexoes existentes so reescrevem o `statusLabel` via `onTemplateChanged`, entao sao
inofensivas como gatilho. **Mas elas registram um limite de design:** se o gatilho por desenho
fosse ligado a `columnIndexSwitched` ou a `frameSwitched` em vez de `xshLevelChanged`, a
insercao passaria a disparar geracao imediatamente - laco de verdade. `xshLevelChanged` e a
escolha certa tambem por isso.

## 4. O que isso implica para a decisao de A3

1. A guarda contra o laco por sinal e barata e continua valendo como protecao futura, mas nao
   corrige nada observavel hoje.
2. O modo de falha real e a **coluna corrente mudar para o nivel gerado**. Uma flag de
   supressao em volta de `onInsertAsLevel()` nao cobre isso, porque o problema persiste depois
   que a flag e desligada.
3. Fechar os dois exige ou um teste sobre o nivel corrente antes de disparar (opcao B de A3),
   ou restaurar a coluna corrente depois da insercao (opcao D de A3), ou ambos.

As alternativas com tradeoffs estao em `aiassist_live_draw__options.md`, secao 4.
