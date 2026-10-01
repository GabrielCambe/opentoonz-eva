# aiassist — fluxo de decisao e contrato de templates

**Atualizado:** 2026-08-29

---

## 1. Por que templates e nao construcao de grafo

O painel em C++ nunca monta um grafo. Ele carrega JSON exportado pelo ComfyUI em
**Save (API Format)** e escreve em campos identificados por `_meta.title`.

Consequencia direta: trocar modelo, adicionar IP-Adapter, migrar para SDXL ou mudar de
sampler vira **largar um arquivo em `graphs/`**. Nenhuma recompilacao. O seletor de grafos
da fase 4 sai quase de graca por causa dessa escolha.

## 2. Varredura e validacao

```
para cada *.json em toonz/sources/aiassist/graphs/
    parse JSON            -> falhou: descarta, registra
    indexa _meta.title que comecam com AI_
    titulo duplicado      -> descarta, registra
    faltam AI_PROMPT, AI_SAMPLER ou AI_OUTPUT -> descarta, registra
    caso contrario        -> entra no seletor
        possui AI_REFERENCE -> consome o frame atual
        nao possui          -> txt2img; desabilita o controle de referencia
```

Templates invalidos nunca chegam ao usuario, e o motivo fica no log. Isso evita a classe de
bug em que o app tenta gerar com um grafo quebrado e falha de forma opaca.

## 3. Decisoes tomadas na captura dos templates

Os grafos vieram do historico do Acer Drawing Assistant e foram limpos antes de virar template.

- **[MEDIUM] `denoise` do txt2img subiu de 0.7 para 1.0.** Partindo de `EmptyLatentImage`
  (latente zerado), `denoise < 1` deixa estrutura residual e lava o resultado. Para geracao
  a partir do nada o valor correto e 1.0. O img2img mantem 0.7, que e o valor calibrado da
  Acer para preservar o traco.
- **[LOW] Um unico no de referencia.** O grafo original tinha dois `LoadImageFromBase64` com
  conteudo identico, um para o `VAEEncode` e outro para o ControlNet, e dois `ImageScale`.
  O template usa um de cada, alimentando os dois caminhos. Semanticamente identico e corta o
  payload pela metade.
- **[LOW] Nos mortos removidos.** O grafo txt2img original carregava a cadeia inteira de
  ControlNet com `strength: 0` e a saida desligada do KSampler, mais um `PreviewImage` sem
  entrada. Tudo isso foi descartado.
- **[LOW] `AI_NEGATIVE` acrescentado ao contrato.** O app da Acer nunca expoe prompt negativo
  e envia string vazia. O campo existe no grafo; expo-lo e ganho sem custo.
- **[LOW] `clip` do prompt negativo padronizado no no da LoRA.** No grafo original o negativo
  do img2img vinha da LoRA e o do txt2img vinha do checkpoint cru — inconsistencia sem
  intencao aparente.

## 4. Regra de portabilidade

Antes de dar qualquer fase por concluida: **isto ainda funciona num PC que nunca ouviu falar
da Acer?**

Se a resposta depende de caminho fixo no codigo, de servico instalado ou do grupo local
`Everyone`, e acoplamento disfarcado. A saida e sempre a mesma: vira preferencia
(`comfyui_url`, `comfyui_launch_cmd`, `comfyui_working_dir`).

Reutilizar o `CreatorSpaceWindowsService` foi avaliado e rejeitado: acopla a maquinas Acer,
ao workaround de locale desta maquina e a um protocolo de pipe nao documentado, sem oferecer
nada alem do que `QProcess` faz em quinze linhas.
