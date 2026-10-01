# ComfyUI — contrato de API para o painel aiassist

**Atualizado:** 2026-08-29
**Escopo:** o que o painel aiassist precisa saber para conversar com um servidor ComfyUI.
**Servidor de referencia:** ComfyUI v0.2.6, Python 3.12.7, cuda:0 RTX 5070 Laptop, 8123 MB de VRAM.

---

## 1. Superficie usada

Sete endpoints. Nenhuma biblioteca cliente e necessaria.

| Metodo | Rota | Uso |
| --- | --- | --- |
| `POST` | `/prompt` | Submete `{"prompt": <grafo>, "client_id": "<uuid>"}`. Retorna `{"prompt_id": ...}`. |
| `GET`  | `/history/<prompt_id>` | Estado e saidas de uma submissao. |
| `GET`  | `/history?max_items=N` | As N entradas mais antigas. Util para inspecao, nao para o fluxo normal. |
| `GET`  | `/view?filename=&subfolder=&type=` | Baixa o PNG. `type` e `temp` para `PreviewImage`, `output` para `SaveImage`. |
| `GET`  | `/object_info/<NodeClass>` | Inventario: modelos disponiveis por tipo de no. |
| `GET`  | `/system_stats` | Dispositivo, VRAM, versao. Bom health check. |
| `POST` | `/interrupt` | Cancela a execucao em voo. Necessario apenas para preview ao vivo. |

O `client_id` e livre; serve para correlacionar eventos no WebSocket `/ws?clientId=`.
O painel na fatia vertical usa polling e nao precisa do WebSocket.

## 2. Ciclo de vida de uma geracao

```
POST /prompt                      -> prompt_id
loop: GET /history/<prompt_id>    ate status.completed == true
      (status.status_str == "error" encerra com falha)
GET /view?...                     -> bytes do PNG
```

Intervalo de polling de 500 ms e suficiente: com LCM em 4 passos o servidor de referencia
completou em menos de 500 ms nos dois testes de validacao.

## 3. Formato do grafo

O corpo de `prompt` e o **formato API** do ComfyUI, obtido na interface por
**Save (API Format)**. E um dicionario `node_id -> no`, sem coordenadas nem informacao de UI:

```json
"31": {
  "class_type": "KSampler",
  "_meta": { "title": "AI_SAMPLER" },
  "inputs": { "seed": 0, "steps": 4, "cfg": 1.0, "sampler_name": "lcm",
              "model": ["38", 0], "positive": ["25", 0], "latent_image": ["10", 0] }
}
```

Valores literais sao escalares. Ligacoes sao `[node_id, output_index]`.
O campo `_meta.title` e livre e ignorado pelo servidor — e o que o painel usa como
contrato de patch.

## 4. Contrato de patch por titulo

O painel nunca constroi grafo. Ele carrega um template e escreve em nos identificados
por `_meta.title`.

| Titulo | Classe esperada | Campo | Obrigatorio |
| --- | --- | --- | --- |
| `AI_PROMPT` | `CLIPTextEncode` | `inputs.text` | sim |
| `AI_SAMPLER` | `KSampler` | `inputs.seed` | sim |
| `AI_OUTPUT` | `PreviewImage` / `SaveImage` | no observado em `outputs` | sim |
| `AI_NEGATIVE` | `CLIPTextEncode` | `inputs.text` | nao |
| `AI_REFERENCE` | `LoadImageFromBase64` | `inputs.data` (base64 sem prefixo data:) | nao |
| `AI_CONTROL` | `ControlNetApply` | `inputs.strength` | nao |

**A ausencia de `AI_REFERENCE` e semantica**, nao um erro: significa que o template e
txt2img e nao consome o canvas. O painel deve desabilitar o controle de referencia nesse caso.

Titulo duplicado e erro de template. Titulo obrigatorio ausente reprova o template na
varredura e ele nao aparece no seletor.

## 5. Inventario do servidor de referencia

| Tipo | Disponivel |
| --- | --- |
| checkpoints | `DreamShaper_8_pruned.safetensors` |
| loras | `pytorch_lora_weights.safetensors` (LCM) |
| controlnet | `control_v11p_sd15_scribble.pth` |
| style_models | nenhum |

Consequencia: os parametros do KSampler nos templates SD 1.5 estao amarrados ao LCM
(`steps 4`, `cfg 1.0`, `sampler lcm`). Um template que troque a LoRA precisa trocar
esses tres valores junto.

## 6. Validacao executada

Round trip completo contra o servidor vivo, com o mesmo contrato de patch do oraculo:

| Template | prompt_id | Resultado |
| --- | --- | --- |
| `sd15_scribble_txt2img` | `fc480acd-…` | PNG de 404.882 bytes |
| `sd15_scribble_img2img` | `0698c065-…` | PNG de 269.649 bytes, referencia de 14.506 chars base64 consumida |

## 7. Modos de falha a tratar no cliente

- **Servidor inalcancavel** — `POST /prompt` falha na conexao. Exibir a URL e apontar para
  as preferencias. Nunca retry silencioso: foi exatamente esse o bug do Acer Drawing Assistant.
- **Grafo recusado (HTTP 400)** — o corpo traz o no e o campo problematico. Propagar o texto.
- **`status_str == "error"`** — falha durante a execucao (modelo ausente, OOM). O objeto de
  status carrega a mensagem.
- **`AI_OUTPUT` sem imagem** — template aponta para um no que nao produz saida de imagem.

## Por que o prompt negativo nao afeta os templates LCM

Os dois templates capturados do Acer Drawing Assistant rodam LCM: `sampler_name: "lcm"`,
`steps: 4`, `cfg: 1.0`, mais uma LoRA de destilacao. Nessa configuracao o no `AI_NEGATIVE`
existe, esta ligado na entrada `negative` do KSampler e recebe o texto normalmente, mas o
resultado nao muda.

A causa e o classifier-free guidance. O sampler calcula:

    ruido = uncond + cfg * (cond - uncond)

Com `cfg = 1.0` isso vira `ruido = cond`. O termo `uncond` - que e onde o prompt negativo
entra - se cancela algebricamente. O negativo e codificado pelo CLIP e descartado.

Nao e escolha da Acer nem bug nosso: LCM e destilado justamente para dispensar guidance
externo e so funciona perto de `cfg = 1`. Subir o cfg num grafo LCM degrada a imagem em vez
de habilitar o negativo.

Consequencia de design: prompt negativo e velocidade sao mutuamente exclusivos por grafo, nao
por aplicacao. Por isso existe `sd15_cfg_txt2img.json` (25 steps, cfg 7.0, dpmpp_2m, sem a
LoRA de LCM), onde o negativo funciona ao custo de ser varias vezes mais lento. O seletor de
grafos e o lugar certo para essa escolha.

Regra geral para novos templates: se `AI_NEGATIVE` estiver presente, `cfg` precisa ser > 1
para que ele tenha efeito.

## Modo ao vivo: regenerar enquanto se digita

O Acer Drawing Assistant dispara uma geracao por caractere - o /history dele mostra 143
prompts progressivamente digitados dentro de 200 geracoes. Funciona, mas enfileira trabalho
que ja nasce obsoleto: se a digitacao for mais rapida que a GPU, a imagem fica atras do
texto e a fila cresce.

O painel usa tres pecas em vez disso:

1. **Debounce de 300 ms.** Um QTimer single-shot reiniciado a cada textEdited. Digitar rapido
   nao gera nada; a pausa entre palavras dispara.
2. **Um unico job em voo.** Nunca dois. O ComfyUI enfileira, e fila e o que faz atrasar.
3. **Coalescencia final.** Ao terminar um ciclo, compara o texto do campo com o que foi
   submetido; se mudou, dispara UMA vez com o mais recente. O numero de geracoes passa a ser
   limitado pela GPU, nao pela digitacao.

O estado pendente nao e um flag: e a diferenca entre m_promptField->text() e
m_submittedPrompt. Um flag poderia dessincronizar do texto; a comparacao nao pode.

Nao se interrompe o job em andamento. Com LCM de 4 passos terminar custa menos que a corrida
que /interrupt introduziria. Mesmo assim o ComfyClient carrega um contador de ciclo em
reply->property("aiGeneration"): resposta de ciclo vencido e descartada, para que um /view
atrasado nunca sobrescreva uma imagem mais nova.

Falha desliga o modo ao vivo em vez de tentar de novo. Servidor fora do ar com retry
automatico a cada 300 ms seria o mesmo defeito do cliente da Acer, so que nosso.

Gate: GraphTemplate::isLiveCapable() exige steps entre 1 e 8. Um grafo de 25 passos levaria
segundos por tecla. Por que o cache do ComfyUI torna isto viavel: mudar so o texto do
CLIPTextEncode invalida aquele no e os seguintes (encode, sampler, VAE decode); checkpoint,
LoRA e VAE continuam carregados em VRAM.

Seed fixa e requisito, nao detalhe. Com seed variando, cada tecla daria uma composicao
diferente e a imagem viraria ruido em vez de responder ao texto.
