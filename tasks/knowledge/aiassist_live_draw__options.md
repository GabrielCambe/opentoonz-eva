---
name: aiassist_live_draw__options
description: Alternativas e tradeoffs das ambiguidades A1 a A4 do modo "regenerate as I draw", ancoradas no codigo com file:line.
sources: task aiassist-live-draw, fase p1
---

# Opcoes de design - "Live: regenerate as I draw"

**Criado:** 2026-09-03 | **Task:** `tasks/aiassist_live_draw.xml` | **Fase:** p1

## 1. Achado do p1-s1: com que granularidade `xshLevelChanged` dispara ao desenhar

Pergunta do plano: dispara por traco, por pincelada, ou so ao soltar o pincel? A resposta
decide toda a calibragem de A1.

**Resposta: uma vez por traco concluido, no momento em que o ponteiro e solto.** Nao ha
emissao por evento de movimento.

Cadeia verificada:

1. O sinal so existe atras de um metodo. `TXshLevelHandle::notifyLevelChange()` e o unico
   lugar que emite - `toonz/sources/include/toonz/txshlevelhandle.h:40`, com o sinal
   declarado em `:47`. Nao ha nenhum outro `emit xshLevelChanged` na arvore.
2. No caminho do pincel quem chama isso e `TTool::notifyImageChanged()`, que termina em
   `m_application->getCurrentLevel()->notifyLevelChange()` -
   `toonz/sources/tnztools/tool.cpp:779` (e a sobrecarga sem fid em `:734`).
3. O pincel raster chama `notifyImageChanged(frameId)` dentro do ramo "finish painting" de
   `ToonzRasterBrushTool::inputSetBusy(bool)` -
   `toonz/sources/tnztools/toonzrasterbrushtool.cpp:1301`, funcao aberta em `:1180`, ramo de
   fim em `:1257`. O pincel vetorial faz o equivalente em
   `toonz/sources/tnztools/toonzvectorbrushtool.cpp:826` (mesma funcao) e `:921`.
4. Quem decide o `busy=false` e o gerenciador de entrada, e ele so o faz quando **todos os
   tracos terminaram**: `toonz/sources/tnztools/inputmanager.cpp:276-280`
   (`if (allFinished) { ... inputSetBusy(false); }`). O `busy=true` correspondente esta em
   `:258`.
5. O caminho por ponto - `inputPaintTracks` / `inputPaintTrackPoint`
   (`toonzrasterbrushtool.cpp:1308`) - **nao** notifica nada. Confirmado por varredura: nao
   ha chamada `notify*` nesse corpo.

**Emissao extra no inicio do traco.** Quando o traco cria conteudo novo - primeira celula da
coluna, frame vazio, "drawing on the cell with red numbers" - o `touchImage()` tambem emite:
`tool.cpp:390`, `:461`, `:474`, `:565`, `:607`. Entao o primeiro traco de uma celula nova
emite **duas** vezes (criacao + fim do traco), e os tracos seguintes emitem uma. O debounce
single-shot ja absorve isso: duas emissoes em poucos milissegundos reiniciam o mesmo timer e
viram um disparo so.

### O que isso significa para A1

A pausa nao e medida "durante o traco", e sim **a partir do fim do ultimo traco**. Isso muda o
enquadramento da pergunta: nao existe risco de disparar no meio de uma pincelada. O risco e
disparar entre dois tracos de um mesmo gesto - por exemplo ao hachurar, onde os tracos se
sucedem rapido.

Referencia de custo, medida na task `sdxl-setup` (secao 5 de `aiassist__sdxl_notes.md`):
`sd15_scribble_img2img` leva 1,2 s. Um disparo indevido custa ~1,2 s de GPU e, por I1, bloqueia
o proximo disparo enquanto roda.

## 2. A1 - qual pausa depois do ultimo traco

Os 300 ms atuais (`aiassistcontroller.cpp:100`) foram calibrados para a pausa entre palavras
digitadas. As alternativas:

| Opcao | Valor | O que acontece ao hachurar | Latencia percebida ao parar |
| --- | --- | --- | --- |
| A | 300 ms, o mesmo do texto | tracos rapidos de um mesmo gesto disparam geracoes que a coalescencia depois descarta | minima |
| B | ~600 ms | um gesto de varios tracos curtos tende a virar um disparo so | pequena, ainda parece "ao vivo" |
| C | ~1000 ms | um gesto inteiro quase sempre vira um disparo so | comeca a parecer que "nao respondeu" |
| D | dois intervalos: 300 ms para texto, valor proprio para desenho | cada gatilho na sua escala | exige escolher o intervalo no ponto de disparo |

Nota sobre a D: o custo de implementacao e baixo. `QTimer::start(int)` aceita o intervalo na
chamada, entao da para manter **um** timer e so escolher o intervalo em cada gatilho. Nao
exige widget nova nem preferencia persistente (o Intent Brief lista preferencia persistente
como nao-objetivo, secao 3).

Esta e a pergunta do p1-s4: depende de como o usuario desenha, nao de leitura de codigo.

## 3. A2 - um checkbox ou dois

Estado atual: um unico `m_liveCheck` com rotulo "Live: regenerate as I type"
(`aiassistcontroller.cpp:92`), guardado por `liveIsActive()` (`:213-219`).

**Opcao A - um checkbox so, rotulo ajustado** (ex.: "Live: regenerate as I draw or type").

- A favor: uma superficie de UI. `liveIsActive()` continua sendo o unico predicado, entao I3
  ("com o Live desligado, nenhum trabalho extra") fica garantido por construcao, num lugar so.
  O pedido do usuario foi "acrescentar" o gatilho, nao "separar".
- Contra: quem quer so o gatilho por texto perde essa opcao. Desenhar com o painel aberto passa
  a custar GPU mesmo sem intencao de regenerar.

**Opcao B - dois checkboxes independentes** ("regenerate as I type" e "regenerate as I draw").

- A favor: controle fino. Da para desenhar com o Live de texto ligado sem disparar.
- Contra: quatro combinacoes de estado para descrever no `statusLabel`, o que torna SC6 bem
  mais dificil de cumprir sem texto confuso. Mais uma linha no formulario. E `liveIsActive()`
  vira dois predicados - duas chances de errar I3 em vez de uma. Tambem exige decidir o que
  `onTemplateChanged` faz com cada um quando o grafo nao e live-capable (`:270-272` hoje
  desmarca o unico check).

**Opcao C - um checkbox mais um seletor de gatilho** (combo: texto / desenho / ambos).

- A favor: mesmo controle da B com uma linha so no formulario.
- Contra: mais widget para uma escolha que provavelmente e feita uma vez e nunca mais. E o
  estado "nenhum dos dois" some, o que na pratica ja e o proprio checkbox desligado.

## 4. A3 - guarda contra o laco de realimentacao

O mapeamento completo do caminho de insercao esta em `aiassist_live_draw__loop_paths.md`. O
resumo que importa aqui: **`Insert as Level` nao emite `xshLevelChanged`**, entao o laco por
sinal nao existe hoje. O que existe e outro problema, de conteudo: a insercao **move a coluna
corrente para a coluna do PNG gerado**, e a coluna corrente e justamente de onde sai a
referencia.

**Opcao A - flag de supressao em volta da insercao.**
Um `bool m_suppressLiveTrigger` ligado no inicio de `onInsertAsLevel()` e desligado no fim; a
lambda do gatilho retorna cedo enquanto ele estiver ligado.

- A favor: trivial, local, e independe de qual sinal a insercao venha a emitir - protege
  tambem se uma versao futura de `IoCmd::loadResources` passar a notificar.
- Contra: cobre so a janela sincrona. Se algo na insercao agendar um sinal para o proximo ciclo
  do event loop, a flag ja estara desligada. (Pelo mapeamento, hoje nao agenda.)
- Contra: nao resolve o problema de conteudo - a coluna corrente continua indo para o PNG.

**Opcao B - teste sobre o nivel corrente.**
Antes de disparar, verificar se o nivel corrente e um `aiassist_*.png` na pasta `+drawings` e,
se for, nao disparar.

- A favor: resolve o problema de conteudo tambem - depois de inserir, desenhar na coluna do
  resultado nao realimenta o gerador com a propria saida.
- Contra: guarda por convencao de nome de arquivo, que quebra se o usuario renomear o nivel. E
  passa a impedir um uso legitimo: desenhar por cima do resultado inserido e regenerar.
- Contra: custa um `getPath()` por disparo - barato, mas nao e nada.

**Opcao C - as duas juntas.** Flag para a janela da insercao, teste de nivel para o estado
posterior. Custo somado ainda baixo, e e a unica que fecha os dois modos de falha.

**Opcao D - devolver a coluna corrente ao fim da insercao.** Guardar o indice de coluna antes
de `IoCmd::loadResources` e restaura-lo depois.

- A favor: ataca a causa, nao o sintoma. Sem ela o painel muda o foco de edicao do usuario sem
  ele pedir - comportamento ja discutivel hoje, antes desta task.
- Contra: muda comportamento existente de `Insert as Level`, fora do escopo declarado (o Intent
  Brief so pede que a insercao **nao dispare** geracao, SC3). Se for desejavel, vira Follow-up.

## 5. A4 - `isLiveCapable()` por passos ou temporal

Sem alternativas novas: o Intent Brief (secao 5, A4) e as notas da raiz do plano ja registram a
decisao de **adiar**. O criterio vive em `GraphTemplate::isLiveCapable()` e e consultado por
`liveIsActive()` (`aiassistcontroller.cpp:216-218`), que e comum aos dois gatilhos - troca-lo
mexeria no gatilho por texto, que funciona hoje. Escopo separado.
