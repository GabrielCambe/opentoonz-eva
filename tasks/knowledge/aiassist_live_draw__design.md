---
name: aiassist_live_draw__design
description: As decisoes de design do modo "regenerate as I draw" - pausa, superficie de UI, guarda contra realimentacao, criterio live-capable e a guarda de prompt vazio - com a razao de cada uma.
sources: task aiassist-live-draw, fase p1
---

# Decisoes de design - "Live: regenerate as I draw"

**Criado:** 2026-09-03 | **Task:** `tasks/aiassist_live_draw.xml` | **Fase:** p1 (p1-s5)
**Insumos:** `aiassist_live_draw__options.md`, `aiassist_live_draw__loop_paths.md`,
`aiassist_live_draw__intent.md`

Este documento e o insumo das fases p2, p3 e p4. As decisoes D1 a D3 sao as ambiguidades A1 a
A3 do Intent Brief, resolvidas com o usuario em 2026-09-03 20:54. D4 e o adiamento de A4. D5
nasceu do proprio p1: e a sub-decisao que o passo p2-s2 delega a "conforme o design do p1".

## D1 - Pausa de 600 ms, so para o gatilho por desenho

**Decisao.** O gatilho por desenho arma o debounce com 600 ms. O gatilho por texto continua com
300 ms. Um unico `QTimer` atende os dois: `QTimer::start(int)` aceita o intervalo na chamada,
entao o intervalo e escolhido no ponto de disparo e nao no construtor.

**Razao.** O sinal chega **uma vez por traco concluido**, na soltura do ponteiro
(`aiassist_live_draw__options.md` secao 1, cadeia verificada de `inputmanager.cpp:276` ate
`txshlevelhandle.h:40`). Entao a pausa mede o intervalo entre dois tracos, nao algo dentro de
um traco. Ao hachurar, os tracos se sucedem em muito menos de 600 ms, e 300 ms produziria
disparos que a coalescencia depois descarta - ~1,2 s de GPU cada um em `sd15_scribble_img2img`
(`aiassist__sdxl_notes.md` secao 5) e, por I1, cada um bloqueando o proximo. 1000 ms foi
descartado pelo usuario por custar sensacao de resposta.

**Confirmado no uso, 2026-09-03.** O usuario desenhou no build e aprovou os 600 ms sem pedir
ajuste. O valor final e 600 ms (`kDrawDebounceMs`). A calibragem esta fechada.

**Sem preferencia persistente.** O Intent Brief lista isso como nao-objetivo (secao 3) e o
comentario em `aiassistcontroller.cpp:56` ja marca a migracao para `Preferences` como trabalho
de outra fase.

## D2 - Um unico checkbox Live, com o rotulo ajustado

**Decisao.** Continua existindo um `m_liveCheck` so. O rotulo passa de
"Live: regenerate as I type" para um texto que cobre os dois gatilhos - "Live: regenerate as I
draw or type" (`aiassistcontroller.cpp:92`).

**Razao.** `liveIsActive()` (`aiassistcontroller.cpp:213-219`) continua sendo o **unico**
predicado que autoriza trabalho ao vivo. Isso e o que faz o invariante I3 - com o Live
desligado o painel nao rasteriza, nao converte para base64, nao escuta caro - ser garantido num
lugar so, em vez de depender de dois predicados corretos ao mesmo tempo. Dois checkboxes
tambem gerariam quatro combinacoes de estado para o `statusLabel` descrever, o que torna SC6
dificil de cumprir sem texto confuso. E o pedido original foi acrescentar um gatilho, nao
separar dois modos.

**Consequencia aceita.** Quem quiser so o gatilho por texto perde essa opcao: com o Live ligado,
desenhar passa a custar GPU. Se isso incomodar no uso, vira Follow-up, nao retrabalho desta
task.

## D3 - Flag de supressao **mais** teste sobre o nivel corrente

**Decisao.** Duas guardas, nao uma:

1. `bool m_suppressLiveTrigger`, ligado na entrada de `onInsertAsLevel()`
   (`aiassistcontroller.cpp:655`) e desligado na saida - inclusive nos caminhos de erro. A
   lambda do gatilho por desenho retorna antes de qualquer trabalho enquanto ele estiver ligado.
2. Um teste antes de disparar: se o nivel corrente e um `aiassist_*.png` na pasta `+drawings`,
   nao disparar.

**Razao, e por que a premissa do Intent Brief mudou.** O p1-s2 mapeou o caminho inteiro e
achou o contrario do que I2 temia:

- **O laco por sinal nao existe.** `IoCmd::loadResources` (`iocommand.cpp:2465-2729`) nao toca
  o level handle em ponto nenhum, e o destrutor do scoped block (`:2437-2449`) emite
  `xsheetChanged`, `castChange` e `onImageChanged`, nunca `notifyLevelChange`. Como
  `notifyLevelChange()` e o unico emissor possivel de `xshLevelChanged`
  (`txshlevelhandle.h:40`), inserir nao pode disparar o gatilho novo.
- **O problema real e de conteudo.** `beforeCellsInsert` (`iocommand.cpp:342-362`) insere uma
  coluna nova quando a celula de destino esta ocupada - o caso normal logo depois de desenhar -
  e **move a coluna corrente para ela** (`:355`). A referencia sai exatamente da coluna
  corrente (`resolveCurrentCell`, `aiassistcontroller.cpp:446-455`). Entao, depois de inserir,
  a proxima geracao usaria a saida da IA como sua propria entrada. Nada dispara sozinho, mas
  degrada a cada disparo seguinte - inclusive pelo gatilho por texto, que ja existe hoje.

A flag sozinha nao cobre isso, porque o problema persiste depois que ela e desligada. O teste
de nivel sozinho abriria mao da protecao contra um sinal futuro vindo da insercao. Por isso as
duas: a flag e barata e e defesa em profundidade; o teste e o que fecha o modo de falha real.

**Custo aceito do teste.** Ele guarda por convencao de nome de arquivo, entao quebra se o
usuario renomear o nivel, e impede um uso legitimo (desenhar por cima do resultado inserido e
regenerar). Ambos sao preferiveis a realimentar o gerador com a propria saida em silencio.

**Fora de escopo, virou Follow-up.** Restaurar a coluna corrente depois da insercao ataca a
causa - hoje o painel muda o foco de edicao do usuario sem ele pedir - mas altera comportamento
existente de `Insert as Level`, que o Intent Brief nao pediu (SC3 so exige que inserir nao
dispare geracao). Registrado no ledger de Follow-ups da raiz do plano.

## D4 - `isLiveCapable()` continua por passos

**Decisao.** Adiar. Nenhuma mudanca nesta task.

**Razao.** O criterio e consultado por `liveIsActive()` (`aiassistcontroller.cpp:216-218`), que
e comum aos dois gatilhos. Troca-lo mexeria no gatilho por texto, que funciona hoje - e por
D2 ele passa a ser o unico predicado de I3. Escopo separado, sem beneficio para SC1 a SC6.

## D5 - Prompt vazio: relaxar so para o gatilho por desenho

**Contexto.** `maybeGenerateLive()` hoje retorna cedo se o prompt esta vazio
(`aiassistcontroller.cpp:223`). Desenhando num grafo `scribble`/img2img, um pedido sem prompt
ainda faz sentido - o desenho e a entrada. O passo p2-s2 do plano delega esta escolha a este
documento, e avisa para decidir, nao seguir por inercia.

**Decisao.** Guardar a origem do ultimo disparo num membro (`Text` ou `Drawing`), definido na
lambda de cada gatilho antes de armar o debounce. Em `maybeGenerateLive()`:

- origem `Text` e prompt vazio -> retorna, **exatamente como hoje**;
- origem `Drawing` e prompt vazio -> so gera se o grafo consome referencia
  (`tpl.consumesReference()`) **e** o `m_useReferenceCheck` esta marcado;
- prompt nao vazio -> como hoje, nos dois casos.

**Razao.** Sem distinguir a origem, relaxar a guarda mudaria tambem o caminho do texto: apagar
o prompt inteiro passaria a disparar uma geracao, e SC5 exige que o gatilho por digitacao
continue **exatamente** como hoje. A condicao extra sobre a referencia evita o caso sem
sentido: um grafo txt2img, sem prompt e sem referencia, geraria ruido puro.

**Bonus.** O mesmo membro resolve D1: e ele que diz qual intervalo usar ao armar o debounce,
inclusive quando quem re-arma e `coalesceLive()` no fim de um ciclo.

## D6 - Serial de revisao do desenho, e nao hash da referencia

**Contexto.** O plano (p3-s1) sugeria guardar a identidade da referencia submetida como um hash
do base64, "que ja e produzido no ciclo, entao nao exige rasterizar de novo".

**Decisao.** Guardar um contador monotonico - `m_drawSerial`, incrementado uma vez por traco no
mesmo ponto em que o gatilho e armado - e `m_submittedDrawSerial`, gravado junto com
`m_submittedPrompt` no `onGenerate()`.

**Razao.** O hash resolve so metade da comparacao. `coalesceLive()` precisa saber se o desenho
mudou **desde** a submissao, e para isso teria de comparar o hash submetido com o hash do estado
**atual** - que nao existe pronto: exigiria rasterizar e recodificar o desenho a cada fim de
ciclo, no caminho comum aos dois gatilhos, para responder o que o proprio sinal
`xshLevelChanged` ja responde de graca. O serial da a mesma identidade por um incremento de
inteiro.

**Custo aceito.** Desenhar e desfazer de volta ao estado identico avanca o serial e gera uma
geracao a mais. O hash evitaria esse caso; nao vale uma rasterizacao por ciclo.

## D7 - O teste de nivel gerado guarda so o gatilho por desenho

**Decisao.** `currentLevelIsGenerated()` bloqueia o gatilho por **desenho**. O gatilho por texto
nao passa por ele.

**Razao.** SC5 exige que a digitacao continue funcionando exatamente como hoje, e o gatilho por
texto e anterior a esta task: guarda-lo seria mudar comportamento existente, nao evitar uma
regressao. Alem disso, o modo de falha que importa e o automatico - desenhar sobre o resultado
inserido e ve-lo se realimentar. Digitar depois de inserir e um ato deliberado, com o painel a
vista.

**Residual, registrado nos Follow-ups.** Depois de `Insert as Level`, digitar ainda gera usando
o PNG inserido como referencia. Fechar isso exigiria restaurar a coluna corrente ao fim da
insercao, que e o Follow-up ja aberto.

## D8 - Segunda fonte de gatilho: `TUndoManager::historyChanged`

**Contexto.** Na verificacao, o usuario observou que `Ctrl+Z` depois de um traco nao regenerava,
e declarou o criterio mais largo que virou SC7: qualquer alteracao do desenho deve regenerar.

**Causa.** O undo das ferramentas passa por `ToolUtils::TToolUndo::notifyImageChanged()`
(`toonz/sources/tnztools/toolutils.cpp:620`), que chama `tool->onImageChanged()` e invalida
icones, mas **nao** chama `notifyLevelChange()`. O level handle so e notificado quando o undo
criou ou removeu frame/level (`:617`, `if (m_createdFrame || m_createdLevel)`). Ou seja: desfazer
um traco num frame que ja existia nao emite `xshLevelChanged`, que era a unica fonte do gatilho.

**Decisao.** Acrescentar `TUndoManager::historyChanged` como segunda fonte, ligada ao **mesmo**
metodo `onDrawingChanged()` que ja carrega as guardas. O sinal e emitido em undo, redo, no
registro de novos undos e no fim de bloco (`toonz/sources/common/tcore/tundo.cpp:259`, `:288`,
`:331`), entao cobre Ctrl+Z e Ctrl+Y de uma vez.

**Alternativa descartada: chamar `notifyLevelChange()` no undo do OpenToonz.** Seria o conserto
na origem, mas alteraria codigo fora do painel e mudaria o comportamento de todos os outros
ouvintes de `xshLevelChanged` - filmstrip, viewer, previewer. Custo desproporcional, e quebraria
o limite que esta task se impos de so tocar `aiassistcontroller.cpp`/`.h`.

**Custo aceito: o sinal e mais largo que o desenho.** Qualquer operacao que registre undo passa
por `onDrawingChanged()`. Isso e proximo do que foi pedido ("quaisquer alteracoes"), mas inclui
operacoes que nao mudam o desenho corrente. Tres coisas limitam o estrago: as guardas de
`onDrawingChanged()` filtram Live desligado, janela de insercao e nivel gerado; o debounce funde
a emissao dupla de um mesmo traco (`xshLevelChanged` + `historyChanged`); e I1 mantem uma geracao
em voo. Se na pratica disparar demais, o proximo passo e testar a identidade do desenho antes de
armar - e ai o hash descartado em D6 passa a valer, porque seria calculado no gatilho e nao a
cada fim de ciclo.

**Efeito colateral bem-vindo.** `Insert as Level` registra undo, entao agora ele **passa** por
`onDrawingChanged()`. A flag `m_suppressLiveTrigger` de D3, que era defesa em profundidade contra
um risco que nao existia, virou guarda ativa: sem ela, inserir dispararia geracao e SC3 quebraria.

**Fechamento, verificado em 2026-09-03.** O usuario confirmou no build: `Ctrl+Z` regenera (SC7) e
`Insert as Level` continua sem disparar (SC3 sob o caminho novo). A previsao acima se confirmou -
a flag de D3, decidida contra um laco por sinal que nao existia, e o que sustenta SC3 agora que o
sinal existe. Vale registrar por que: D3 foi escolhida como defesa em profundidade justamente por
nao depender de qual sinal a insercao viesse a emitir, e foi essa propriedade, e nao a analise do
`IoCmd`, que sobreviveu a mudanca.

**Nao verificado:** se `historyChanged` dispara demais no uso prolongado. O usuario nao relatou
incomodo, mas tambem nao foi o alvo do teste. Se aparecer, o proximo passo esta escrito acima -
testar a identidade do desenho antes de armar, com o hash descartado em D6 calculado no gatilho.

## Resumo para as fases seguintes

| # | Decisao | Onde incide |
| --- | --- | --- |
| D1 | 600 ms para desenho, 300 ms para texto, um timer so | p2 (conexao), p4 (texto de status), p4-s6 (registrar o valor final) |
| D2 | Um checkbox, rotulo cobrindo os dois gatilhos | p2 |
| D3 | Flag de supressao + teste de nivel `aiassist_*.png` | p4 |
| D4 | `isLiveCapable()` inalterado | nenhuma |
| D5 | Origem do disparo num membro; prompt vazio so vale para desenho com referencia | p2, e reaproveitado no p3 |
| D6 | Serial de revisao do desenho em vez de hash da referencia | p3 |
| D7 | O teste de nivel gerado guarda so o gatilho por desenho | p4 |
| D8 | historyChanged como segunda fonte, no mesmo onDrawingChanged() | p5 |

## Resultado da verificacao

**Estado em 2026-09-03: SC1 a SC7 verificados pelo usuario no build. Verificacao completa.**

### Verificado

- **SC1 a SC6, pelo usuario, desenhando no build (2026-09-03).** Os seis criterios do roteiro do
  p4-s4 se comportaram como esperado. A pausa de 600 ms foi aprovada sem pedido de ajuste.
- **O que a mesma sessao revelou:** `Ctrl+Z` depois de um traco nao regenerava. Virou SC7 e a
  fase p5; a causa esta em D8.
- **SC7 e SC3 sob o caminho novo, pelo usuario (2026-09-03).** Depois de ligar
  `TUndoManager::historyChanged`: `Ctrl+Z` regenera, e `Insert as Level` continua sem disparar -
  o que confirma que a flag de supressao esta segurando o caminho que agora existe.
- **Compilacao.** Build Release limpo depois de cada uma das tres fases de codigo (p2, p3, p4),
  sem erro nem warning novo em `aiassistcontroller.cpp`/`.h`. Ultimo binario:
  `toonz/build/Release/OpenToonz.exe`.
- **I4 - grafos e contrato intactos.** Os sete JSON continuam validos e com os mesmos
  marcadores: `sd15_cfg_txt2img` (7 nos), `sd15_scribble_img2img` (14),
  `sd15_scribble_txt2img` (8), `sdxl_base_cnet` (11), `sdxl_base_t2i` (11),
  `sdxl_lightning_cnet` (12), `sdxl_lightning_t2i` (12). Nenhum deles foi tocado.
- **Nenhum arquivo fora do painel.** Os unicos arquivos de codigo modificados na task sao
  `toonz/sources/aiassist/aiassistcontroller.cpp` e `.h`. Conferido por mtime e por
  `git status` - o conjunto de arquivos nao versionados e identico ao do inicio da sessao.
- **SC5 por leitura, no ponto em que a fase p3 mexeu no caminho comum.** Digitando sem
  desenhar, `m_drawSerial` nunca avanca, entao `drawingChanged` e falso e a condicao de
  `coalesceLive()` se reduz exatamente a comparacao de texto que existia antes; `armLive(Text)`
  usa 300 ms, o mesmo intervalo de antes. O gatilho por texto nao mudou.
- **I1 por leitura.** A guarda `m_client->isBusy()` em `maybeGenerateLive()` nao foi tocada por
  nenhuma das tres fases.

### Pendente - so o uso responde

- **SC1, SC2, SC3, SC4, SC6 em execucao.** Exigem uma pessoa desenhando com ponteiro no build.
  O roteiro esta em p4-s4 e cobre os seis criterios de uma vez.
- **SC7 em execucao.** O gatilho por undo/redo foi implementado e compila, mas nao foi exercido:
  falta confirmar que `Ctrl+Z` regenera e que `historyChanged` nao dispara demais no uso normal.
  E tambem re-confirmar SC3, porque `Insert as Level` agora passa pelo caminho do gatilho e
  depende da flag de supressao para nao disparar.

