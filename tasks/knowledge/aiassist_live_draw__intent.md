---
name: aiassist_live_draw__intent
description: Intent Brief do modo "Live: regenerate as I draw" no painel AI Assist. Goal, criterios de sucesso, nao-objetivos, invariantes e o registro de ambiguidades.
sources: task aiassist-live-draw
---

# Intent Brief - "Live: regenerate as I draw"

**Criado:** 2026-09-03
**Task:** `tasks/aiassist_live_draw.xml`
**Origem:** pedido do usuario durante o gate do p5 da task `sdxl-setup`, registrado no ledger
de Follow-ups daquele plano.

## 1. Goal

Hoje o modo ao vivo do painel AI Assist regenera quando o usuario **digita** no prompt. Este
trabalho faz o painel regenerar tambem quando o usuario **desenha** - o desenho corrente vira
gatilho, do mesmo jeito que o texto ja e.

## 2. Criterios de sucesso

- **SC1.** Com o Live ligado e um template rapido selecionado, desenhar um traco no nivel
  corrente dispara uma geracao sem que o usuario toque no prompt nem no botao Generate.
- **SC2.** Continuar desenhando durante uma geracao em voo nao enfileira geracoes: no maximo
  uma esta em voo, e ao terminar dispara **uma** nova se o desenho mudou nesse meio tempo.
- **SC3.** Inserir o resultado com "Insert as Level" nao dispara uma nova geracao.
- **SC4.** Com o Live desligado, nenhum comportamento muda em relacao a hoje.
- **SC5.** O gatilho por digitacao continua funcionando exatamente como hoje.
- **SC6.** A nota de status do painel descreve o gatilho vigente sem mentir - se o gatilho passa
  a ser desenho tambem, o texto "regenerates 300 ms after you stop typing" precisa acompanhar.
- **SC7.** *(acrescentado em 2026-09-03, ver Revisions)* O gatilho e "o desenho corrente mudou",
  e nao "um traco novo foi feito". Desfazer e refazer sao alteracoes do desenho e tambem devem
  regenerar.

## 3. Nao-objetivos

- **Nao** e um preview continuo por traco. O alvo e "regenera pouco depois de eu parar de
  desenhar", nao streaming.
- **Nao** viabiliza SDXL no modo ao vivo. A medicao da task `sdxl-setup` (secao 5 de
  `aiassist__sdxl_notes.md`) mostra 5,8 s no mais rapido dos SDXL contra 1,2 s do SD 1.5.
  O recurso e util na rota SD 1.5; nao e uma tentativa de acelerar SDXL.
- **Nao** mexe nos grafos JSON nem nos pesos. Aquilo foi a task `sdxl-setup`.
- **Nao** adiciona preferencia de usuario persistente para o intervalo de debounce, a menos que
  a decisao do p1 conclua que precisa. O comentario em `aiassistcontroller.cpp:56` ja marca
  "migrar para Preferences" como trabalho de outra fase.

## 4. Invariantes (must-not-break)

- **I1.** No maximo uma geracao em voo. Hoje isso e garantido por `m_client->isBusy()` em
  `maybeGenerateLive()`; a garantia nao pode ser enfraquecida.
- **I2.** Sem laco de realimentacao. Nenhum caminho pode fazer o painel disparar a si mesmo
  indefinidamente. O risco concreto e o `onInsertAsLevel()`, que grava um PNG em `+drawings` e
  chama o carregador do OpenToonz - isso altera o level set e pode emitir o mesmo sinal que
  passa a ser o gatilho.
- **I3.** Com o Live desligado o painel nao pode fazer trabalho extra nenhum - nem rasterizar,
  nem converter para base64, nem escutar sinal caro.
- **I4.** Os tres grafos `sd15_*` e os quatro `sdxl_*` continuam funcionando; nenhuma mudanca
  de contrato de marcadores.

## 5. Ambiguidades registradas

Estas sao as decisoes que o p1 resolve com o usuario. Aqui elas ficam como perguntas, nao como
respostas.

| # | Pergunta | Suposicao provisoria | Confianca |
| --- | --- | --- | --- |
| A1 | Qual pausa depois do ultimo traco? Os 300 ms atuais foram calibrados para digitacao. | Pausa entre tracos e maior que entre palavras; provavelmente 600 a 1000 ms. Decidir com o usuario desenhando. | media |
| A2 | O gatilho por desenho deve ser um segundo checkbox, ou o mesmo "Live" passa a cobrir os dois? | Um so checkbox, com o rotulo ajustado. Menos superficie de UI e o usuario pediu "adicionar", nao "separar". | media |
| A3 | Como evitar o laco do `Insert as Level`? | Ignorar o sinal durante a insercao, ou nao regenerar quando o nivel corrente for um `aiassist_*.png` gerado pelo painel. | baixa |
| A4 | O criterio `isLiveCapable()` deve deixar de ser por passos e virar temporal? | Adiar. Trocar o criterio afeta tambem o gatilho por texto, que funciona hoje; e escopo separado. | media |

## 6. Achado do scan preliminar que muda o trabalho

A implementacao **nao e apenas uma conexao a mais**. `coalesceLive()`
(`aiassistcontroller.cpp:236`) decide se re-dispara comparando
`m_promptField->text() == m_submittedPrompt`: a coalescencia e chaveada no **texto do prompt**.

Com gatilho por desenho o prompt nao muda entre geracoes, entao essa comparacao retorna cedo e
**a coalescencia nunca re-dispara** - o usuario desenharia durante uma geracao e a mudanca seria
perdida em silencio. Cumprir SC2 exige estender o estado de coalescencia para incluir a
identidade da referencia submetida, e nao so o texto.

Esse e o motivo de o plano ter uma fase propria para coalescencia, em vez de tratar tudo como
uma unica edicao.

## 7. Revisions

- 2026-09-03: criado.
- 2026-09-03 21:2x: **SC7 acrescentado, a pedido do usuario no gate de verificacao.** Ao rodar o
  roteiro do p4-s4 ele observou que `Ctrl+Z` depois de um traco nao regenera, e declarou o
  criterio: "e importante que quaisquer alteracoes causem uma regeneracao".

  Isto amplia o gatilho. SC1 falava em *desenhar um traco*, e foi cumprido ao pe da letra; o
  criterio verdadeiro e mais largo - qualquer mudanca do desenho corrente. A causa tecnica esta
  registrada em `aiassist_live_draw__design.md` (D8): o undo das ferramentas passa por
  `ToolUtils::TToolUndo::notifyImageChanged()` (`toolutils.cpp:620`), que invalida icone e
  atualiza a ferramenta mas **nao** chama `notifyLevelChange()` - o level handle so e avisado
  quando o undo criou ou removeu frame/level (`:617`). Por isso o gatilho de p2 nao via o Ctrl+Z.

  Trabalho correspondente: fase p5. Nao e retrabalho de p2 - o que p2 entregou continua valendo
  e passou nos seus criterios; o que mudou foi a definicao do gatilho.
