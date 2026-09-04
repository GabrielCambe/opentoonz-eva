---
name: aiassist_live_draw__explanation
description: O que mudou no painel AI Assist para ele regenerar enquanto se desenha, por que cada decisao foi tomada, e o que a execucao revelou que o plano nao previa.
sources: task aiassist-live-draw
---

# O painel AI Assist agora regenera enquanto voce desenha

**Task:** `tasks/aiassist_live_draw.xml` | **Concluida:** 2026-09-03
**Arquivos de codigo tocados:** dois. `toonz/sources/aiassist/aiassistcontroller.cpp` e `.h`.

## Resumo

O modo Live regenerava quando voce **digitava**. Agora regenera tambem quando voce **desenha** -
e quando voce desfaz. O que parecia "mais um `connect`" acabou exigindo quatro mudancas de
comportamento, porque o gatilho novo atravessa codigo que tinha sido escrito assumindo que o
texto era a unica coisa que mudava.

Duas coisas so apareceram executando, e nenhuma delas apareceria numa revisao de diff:

1. A coalescencia era chaveada no texto do prompt. Desenhando, o prompt nao muda entre geracoes,
   entao ela nunca re-disparava e desenhar durante uma geracao seria perdido em silencio.
2. `Ctrl+Z` nao regenerava, porque o undo das ferramentas do OpenToonz nao notifica o level
   handle.

## 1. Como funcionava antes

O painel (`aiassistcontroller.cpp`) tinha um unico gatilho ao vivo: `QLineEdit::textEdited` no
campo de prompt, ligado a um `QTimer` single-shot de 300 ms. No fim do debounce,
`maybeGenerateLive()` checava tres coisas - modo Live ativo, prompt nao vazio, cliente nao
ocupado - e chamava `onGenerate()`. Ao fim de cada ciclo, `coalesceLive()` comparava o texto
atual com `m_submittedPrompt` e re-disparava se tivessem divergido.

A arquitetura era boa e foi reaproveitada quase inteira. O problema e que **todo o estado de
"ainda ha trabalho pendente" era o texto do prompt**. Era uma suposicao correta enquanto o texto
fosse o unico gatilho, e ela ficou errada no instante em que deixou de ser.

O painel ja sabia capturar o desenho corrente (`currentFrameAsPngBase64()`), ja garantia uma
geracao em voo (`m_client->isBusy()`) e ja escutava o `TXshLevelHandle` - mas em
`xshLevelSwitched`, que dispara ao **trocar** de nivel, nao ao **alterar** o nivel.

## 2. O que mudou

| Mudanca | Efeito |
| --- | --- |
| `onDrawingChanged()`, um caminho unico para "o desenho mudou" | Duas fontes - fim de traco e undo/redo - entram pelas mesmas guardas |
| Duas pausas: 600 ms para desenho, 300 ms para texto | Um gesto de varios tracos vira um disparo, sem tornar a digitacao lenta |
| `m_drawSerial` / `m_submittedDrawSerial` | A coalescencia passa a enxergar mudanca de desenho, nao so de texto |
| `m_suppressLiveTrigger` + `currentLevelIsGenerated()` | A saida da IA nao vira entrada da propria IA |
| Guarda de prompt vazio sensivel a origem | Desenhar sem digitar gera; apagar o prompt continua nao gerando |
| Texto de status reescrito | A nota descreve as duas pausas de verdade |

## 3. Onde mudou

Tudo em `toonz/sources/aiassist/`:

- **`aiassistcontroller.h`** - `enum class LiveTrigger { Text, Drawing }`, os membros
  `m_lastLiveTrigger`, `m_drawSerial`, `m_submittedDrawSerial`, `m_suppressLiveTrigger`, e as
  declaracoes de `armLive()`, `onDrawingChanged()` e `currentLevelIsGenerated()`.
- **`aiassistcontroller.cpp`** - constantes `kTextDebounceMs`/`kDrawDebounceMs`, o helper RAII
  `ScopedFlag`, as duas conexoes novas, o corpo de `onDrawingChanged()`, a condicao estendida de
  `coalesceLive()`, a guarda de prompt vazio em `maybeGenerateLive()`, e o texto de
  `onTemplateChanged()`.

Nenhum outro arquivo de codigo foi alterado. Os sete grafos JSON continuam intactos (I4).

Conhecimento produzido, em `tasks/knowledge/`: `__intent.md` (revisado com SC7),
`__options.md`, `__loop_paths.md`, `__design.md` (oito decisoes), e este arquivo.

## 4. Por que cada decisao

**600 ms para desenho, 300 ms para texto (D1).** O sinal de desenho chega **uma vez por traco
concluido**, na soltura do ponteiro - a cadeia foi rastreada de
`inputmanager.cpp:276` ate `txshlevelhandle.h:40`. Entao a pausa mede o intervalo *entre tracos*,
e ao hachurar eles se sucedem em muito menos de 300 ms. Manter 300 ms geraria disparos que a
coalescencia depois descarta, ~1,2 s de GPU cada. Voce calibrou e aprovou os 600 ms no build.

**Um checkbox, nao dois (D2).** Mantem `liveIsActive()` como o unico predicado que autoriza
trabalho ao vivo, entao o invariante "com o Live desligado o painel nao faz nada" (I3) fica
garantido num lugar so, em vez de depender de dois predicados corretos simultaneamente.

**Serial de revisao, nao hash da referencia (D6).** O plano sugeria hash do base64 submetido. Isso
resolve so metade: comparar exigiria rasterizar e recodificar o desenho a **cada fim de ciclo**,
no caminho comum aos dois gatilhos, para responder o que o proprio sinal ja responde de graca. O
custo aceito: desenhar e desfazer ate um estado identico gera uma vez a mais.

**`historyChanged` em vez de consertar o undo do OpenToonz (D8).** A correcao na origem seria
acrescentar `notifyLevelChange()` ao undo das ferramentas - mais correto conceitualmente, mas
mudaria o comportamento de todo ouvinte de `xshLevelChanged` (filmstrip, viewer, previewer) por
causa de um painel.

## 5. Como funciona agora

```
fim de traco  --> TXshLevelHandle::xshLevelChanged --.
undo / redo   --> TUndoManager::historyChanged ------+--> onDrawingChanged()
                                                     |      |
digitar       --> QLineEdit::textEdited -------------'      | guardas: Live ativo?
                          |                                 |          insercao em curso?
                          |                                 |          nivel gerado?
                          |                                 v
                          |                            ++m_drawSerial
                          |                                 |
                          '------> armLive(Text, 300) <-----' armLive(Drawing, 600)
                                          |
                                    m_debounce (single-shot)
                                          v
                                  maybeGenerateLive()
                                    isBusy? -> nao faz nada
                                          v
                                     onGenerate()
                              grava m_submittedPrompt + m_submittedDrawSerial
                                          v
                                    onImageReady()
                                          v
                                    coalesceLive()
                          texto mudou OU desenho mudou? -> arma de novo
```

O ponto que faz isso funcionar: **`m_drawSerial` avanca no gatilho, nao na geracao**. Se voce
desenha enquanto uma geracao esta em voo, o serial ja avancou quando o ciclo termina, entao
`coalesceLive()` ve a divergencia e dispara exatamente **uma** vez a mais, com o desenho mais
recente. E o que cumpre SC2 sem enfileirar nada.

## 6. Conceitos que importaram

**Debounce com intervalo por origem.** `QTimer::start(int)` **grava** o intervalo no timer. Armar
em um lugar com 600 e em outro com `start()` sem argumento faria o segundo herdar 600 em
silencio. Por isso existe `armLive()` como ponto unico de armacao - a alternativa nao seria um
erro visivel, seria o gatilho de texto ficando lento sem ninguem entender por que.

**Coalescencia versus enfileiramento.** Enfileirar produz imagens que ja nascem obsoletas: e o
defeito diagnosticado no cliente da Acer, cujo `/history` tem 143 prompts progressivamente
digitados em 200 geracoes. Coalescer limita o numero de geracoes pela velocidade da GPU em vez de
pela velocidade da mao. O preco e que o estado pendente precisa ser **completo** - foi
exatamente onde o codigo antigo falhava, cobrindo texto e nao desenho.

**Realimentacao de conteudo sem laco de sinal.** O risco declarado no Intent Brief era um laco:
inserir emitiria o sinal do gatilho, que geraria, que inseriria. O mapeamento mostrou que esse
laco **nao existia** - `IoCmd::loadResources` nunca toca o level handle. O risco real era outro e
mais silencioso: `beforeCellsInsert` (`iocommand.cpp:355`) move a **coluna corrente** para a
coluna do PNG inserido, e a referencia sai justamente da coluna corrente. Nada dispara sozinho,
mas a proxima geracao usaria a saida da IA como entrada. Sistemas realimentados degradam sem
travar - e o modo de falha que nao aparece em log nenhum.

**Defesa em profundidade que virou guarda ativa.** A flag `m_suppressLiveTrigger` foi decidida
(D3) contra um laco por sinal que a analise mostrou nao existir - a justificativa era nao depender
de qual sinal a insercao viesse a emitir. Quando `historyChanged` entrou em p5, `Insert as Level`
passou a registrar undo e portanto a **passar** pelo gatilho. A flag deixou de ser precaucao e
virou o que sustenta SC3. A licao: a guarda sobreviveu porque foi escolhida por uma propriedade
estrutural, nao pela analise pontual do `IoCmd`.

## 7. Evidencia de verificacao

**Verificado por execucao, por voce, no build Release:**

- SC1 desenhar dispara sem tocar no prompt nem no Generate; SC4 Live desligado inalterado.
- SC2 desenhar durante uma geracao nao acumula, e o resultado acompanha o ultimo estado.
- SC3 `Insert as Level` nao dispara - reconfirmado depois de `historyChanged` entrar.
- SC5 digitar continua como antes; SC6 a nota de status descreve as duas pausas.
- SC7 `Ctrl+Z` regenera.
- A pausa de 600 ms aprovada sem pedido de ajuste.

**Verificado mecanicamente:** build Release limpo apos cada fase; sete grafos JSON validos com
marcadores inalterados; apenas os dois arquivos do painel modificados (mtime + `git status`).

**Pontos cegos, declarados:**

- **Se `historyChanged` dispara demais no uso prolongado.** O sinal e mais largo que o desenho:
  qualquer operacao que registre undo passa pelo gatilho. Nao houve queixa, mas tambem nao foi o
  alvo do teste.
- **A guarda de nivel gerado nunca foi exercida com um nivel renomeado.** Ela identifica o
  resultado por nome de arquivo (`aiassist_*.png`), entao renomear o nivel a desarma.
- **Nenhum teste automatizado.** A verificacao inteira e por uso, porque o que mudou e o gatilho
  de uma interface. O oraculo `tools/aiassist_oracle.py` valida grafo e nao serve aqui.

## 8. Riscos remanescentes e proximos passos

**Se `historyChanged` disparar demais.** A saida ja esta escrita em D8: testar a identidade do
desenho antes de armar. E ai o hash descartado em D6 passa a valer, porque seria calculado **no
gatilho** e nao a cada fim de ciclo - o argumento contra ele desaparece nesse contexto.

**Follow-ups registrados durante a execucao (fora do escopo declarado, cada um vira uma task):**

1. **`Insert as Level` move o foco de edicao sem pedir.** `beforeCellsInsert`
   (`iocommand.cpp:355`) chama `setColumnIndex`. Restaurar a coluna corrente ao fim da insercao
   atacaria a **causa** da realimentacao de conteudo, em vez do sintoma. E comportamento anterior
   a esta task. Fecharia tambem o residual do D7: depois de inserir, **digitar** ainda gera usando
   o PNG inserido como referencia.
2. **Com um checkbox so, quem quer apenas o gatilho por texto perde a opcao.** Se incomodar,
   avaliar o combo de gatilho (texto / desenho / ambos) descrito em `__options.md`.
3. **`isLiveCapable()` por passos (A4, adiado).** Trocar por criterio temporal afeta tambem o
   gatilho por texto. Escopo separado, sem beneficio para os criterios desta task.
4. **Intervalo de debounce como preferencia persistente.** Nao-objetivo declarado; o comentario em
   `aiassistcontroller.cpp:56` ja marca a migracao para `Preferences` como trabalho de outra fase.

**Nota de ferramental.** O `indexer.py sync` aceita apenas o **ultimo** `--path` quando varios sao
passados, e ainda assim reporta sucesso (`added: 1`). Duas das tres sincronizacoes iniciais foram
silenciosamente perdidas, e so apareceram porque o caminho de leitura foi conferido com um
`query`. Quem for scriptar isso: uma chamada por arquivo.
