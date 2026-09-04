---
name: aiassist__sdxl_notes
description: Orcamento de VRAM do SDXL na RTX 5070 Laptop, as rotas testaveis e o roteiro de medicao. Ler antes de baixar modelo ou de culpar o painel por lentidao.
sources: cowork
---

# SDXL no painel AI Assist

## 1. Por que isto e uma questao de VRAM, nao de codigo

O painel nao muda nada para rodar SDXL. O contrato e por `_meta.title`, entao um grafo SDXL
com AI_PROMPT, AI_SAMPLER e AI_OUTPUT funciona pela mesma interface. O que muda e se cabe na
placa e se responde rapido o bastante para o modo ao vivo.

**Servidor de referencia:** RTX 5070 Laptop, 8123 MB de VRAM reportados pelo /system_stats.

## 2. Orcamento aproximado, em fp16

| Componente | Tamanho aproximado |
| --- | --- |
| UNet, ~2,6 B parametros | ~5,0 GB |
| Text encoders (CLIP-L + OpenCLIP-G) | ~1,6 GB |
| VAE | ~0,16 GB |
| **Pesos, total** | **~6,8 GB** |

Contra 8123 MB isso deixa pouco mais de 1 GB para ativacoes. A 1024x1024 essa folga e
apertada, e o ComfyUI comeca a fazer offload de partes do modelo para a RAM. Offload nao
quebra nada: torna mais lento.

**Correcao registrada:** uma estimativa anterior nesta conversa falou em ~9,5 GB de pesos.
Estava inflada. O numero que decide, de qualquer forma, e o pico medido - nao a tabela.

Para comparacao, o SD 1.5 que ja roda usa cerca de 4 a 5 GB no total, com folga confortavel.

## 3. As quatro rotas materializadas em grafos

| Template | Passos | cfg | Modo ao vivo | Para que serve |
| --- | --- | --- | --- | --- |
| `sdxl_lightning_t2i.json` | 8 | 1.5 | sim | Explorar digitando |
| `sdxl_lightning_cnet.json` | 8 | 1.5 | sim | Idem, com condicionamento mais forte |
| `sdxl_base_t2i.json` | 28 | 7.0 | nao | Finalizar; negativo funciona |
| `sdxl_base_cnet.json` | 28 | 7.0 | nao | Idem, condicionamento mais forte |

As duas variantes de condicionamento diferem em **uma linha**: `control_net_name` no no
`Load Control Model`. O ComfyUI carrega T2I-Adapter pelo mesmo `ControlNetLoader` que carrega
ControlNet, desde que o arquivo esteja em `models/controlnet/`. Por isso a forma do grafo e
identica nas duas.

| | Tamanho | Efeito |
| --- | --- | --- |
| T2I-Adapter sketch | ~300 MB | Leve; condicionamento mais frouxo, costuma bastar para rabisco |
| ControlNet SDXL scribble | ~2,5 GB | Controle mais forte; come a folga que ja e pequena |

Com 8 GB, somar 2,5 GB de ControlNet aos ~6,8 GB do modelo garante offload. Esperar que a
variante `cnet` seja visivelmente mais lenta que a `t2i` e o comportamento correto, nao um bug.

## 4. Downloads e nomes de arquivo

Os templates trazem os nomes canonicos das distribuicoes oficiais:

| Pasta do ComfyUI | Arquivo esperado pelos templates |
| --- | --- |
| `models/checkpoints` | `sd_xl_base_1.0.safetensors` |
| `models/loras` | `sdxl_lightning_8step_lora.safetensors` |
| `models/controlnet` | `t2i-adapter_diffusers_xl_sketch.safetensors` |
| `models/controlnet` | `controlnet-sdxl-scribble.safetensors` |

**Se o seu download salvar com outro nome, o grafo falha.** Isso nao e silencioso: o ComfyUI
recusa o POST /prompt e o painel mostra qual no foi rejeitado, porque o ComfyClient repassa o
corpo da resposta. Para conferir o que existe de fato:

    GET http://127.0.0.1:8188/object_info/CheckpointLoaderSimple
    GET http://127.0.0.1:8188/object_info/ControlNetLoader

O inventario vem na lista de valores aceitos do primeiro input. Renomear o arquivo baixado
para o nome do template e mais rapido do que editar quatro grafos.

## 5. Roteiro de medicao

Para cada um dos quatro templates, com o mesmo prompt e a mesma seed:

1. Reiniciar o ComfyUI antes de cada rodada, para nao medir um modelo que ja estava em cache.
2. Gerar duas vezes. Anotar a **segunda**: a primeira paga o carregamento do modelo.
3. Anotar o tempo que o proprio painel mostra na linha de status.
4. Anotar o pico de VRAM (Gerenciador de Tarefas, aba Desempenho, GPU) durante a geracao.

| Template | Carregamento + 1a geracao (s) | Geracoes medidas (s) | Pico de VRAM (MiB) | Observacao |
| --- | --- | --- | --- | --- |
| sdxl_lightning_t2i | 10.89 | 5.81 / 5.80 | 7231 (delta 5467 sobre 1764 em repouso) | Custo base do SDXL nesta placa. Carregar o modelo custa ~5 s. 4,8x o tempo do SD 1.5. |
| sdxl_lightning_cnet | 16.89 | 9.92 / 8.89 | 7715 (delta 5942 sobre 1773 em repouso) | ControlNet cobra 1,53x sobre o t2i de mesmos passos. Pico a 408 MiB do teto, mas o tempo ainda escala de forma previsivel: sem sinal de offload. |
| sdxl_base_t2i | 23.35 | 15.60 / 15.51 | 7403 (delta 5686 sobre 1717 em repouso) | 3,5x os passos do lightning para 2,67x o tempo: escala sublinear, como esperado quando o custo fixo (~1,9 s) e amortizado. Sem offload. |
| sdxl_base_cnet | 43.78 | 36.45 / 36.02 | 7705 (delta 6014 sobre 1691 em repouso) | AQUI HA OFFLOAD. Um modelo linear ajustado no par t2i (custo fixo 1,9 s, 0,486 s por passo) e no lightning_cnet (0,872 s por passo) preve 26,3 s. O medido foi 36,0 s: 9,7 s a mais, +37%. Esse excesso e a placa saturada trocando pesos com a RAM. |
| sd15_scribble_img2img (base de comparacao) | 6.84 | 1.19 / 1.20 | 5040 (delta 3325 sobre 1715 em repouso) | Unica linha que cabe no modo ao vivo: 1,2 s contra o limiar de ~2 s. Usa 2,4 GB a menos de VRAM que qualquer rota SDXL. |

**Como estes numeros foram obtidos, e onde o roteiro acima estava errado.** Cada linha
vem de um reinicio real do ComfyUI, seguido de uma geracao de aquecimento com a seed 42 -
que paga o carregamento do modelo - e de duas geracoes medidas com as seeds 43 e 44, ja
com o modelo residente. A coluna traz as duas, nao uma media, para a dispersao ficar
visivel.

O roteiro original pedia duas geracoes *com a mesma seed* e mandava anotar a segunda.
Isso nao mede nada neste servidor: o ComfyUI tem cache de execucao e um segundo
`POST /prompt` com grafo identico volta do cache sem rodar o sampler. Na primeira
tentativa a "segunda geracao" saiu em 0,18 s com uma unica amostra de VRAM - nao e uma
geracao rapida, e um cache hit. A seed identica era incidental; o que o passo queria era
uma execucao real sem o custo de carregar o modelo, e e isso que a seed nova entrega.

O pico de VRAM e da GPU inteira, por `nvidia-smi` amostrado a cada 200 ms, entao inclui a
area de trabalho e o que mais estiver aberto. Por isso a coluna traz tambem o delta sobre
a leitura em repouso feita logo apos o reinicio, que e o numero atribuivel ao modelo.


O numero que decide o modo ao vivo e a segunda geracao da linha `lightning`. Se passar de
uns 2 s, digitar deixa de ser fluido e o debounce de 300 ms precisa subir - ou a rota
lightning nao vale a pena nesta maquina.

## 6. Pontos que provavelmente vao precisar de ajuste

- **denoise 0.75** nos templates img2img e um chute inicial. Mais alto obedece menos ao
  desenho, mais baixo obedece mais e inventa menos.
- **cfg 1.5** do Lightning: a distilacao pede cfg baixo, mas o Lightning tolera um pouco mais
  que o LCM. Ainda assim o prompt negativo fica praticamente inutil ali - o painel vai
  desabilitar o campo, pelo mesmo criterio de sempre.
- **sgm_uniform** e o scheduler recomendado para SDXL-Lightning. Se o resultado sair lavado,
  esse e o primeiro parametro a mexer.
- **1024x1024** e a resolucao nativa do SDXL. Gerar a 512 no SDXL costuma piorar, nao acelerar.
  Se a camera da cena nao for quadrada, o aviso de esticamento do painel vai disparar - e ai
  vale editar o `AI_SCALE` para a proporcao da sua camera.

## 7. Veredito, 2026-09-03

Escrito depois de instalar os pesos, gerar com os quatro templates e medir tempo e VRAM
nesta maquina. Substitui as expectativas das secoes 2 e 3 onde elas divergirem: o que
manda e o numero medido.

### O SDXL vale a pena aqui?

**Sim, mas so na variante ControlNet, e nao para o modo ao vivo.**

**Para explorar digitando: continue no SD 1.5.** O modo ao vivo nao fecha com SDXL. O
`sdxl_lightning_t2i`, a rota mais rapida das quatro, faz 5,8 s por geracao com o modelo
ja residente. O limiar da secao 5 e ~2 s, e o `sd15_scribble_img2img` faz 1,2 s. Nao e
questao de subir o debounce de 300 ms: faltam quase 3x. Uma geracao a cada 6 s nao e
"regenerar enquanto digito", e um botao de gerar com espera.

**Para finalizar: `sdxl_base_cnet`, com denoise 0.95.** E o unico dos cinco que produz uma
ilustracao de verdade a partir do rabisco - parede de pedra, porta e telhado com cor e
materia. Custa 36 s por imagem, que e aceitavel para um quadro final e proibitivo para
qualquer outra coisa.

**Meio-termo: `sdxl_lightning_cnet`, com denoise 0.95.** ~9 s, resultado limpo e ja
colorido. E a melhor troca se 36 s incomodar.

**As duas variantes T2I-Adapter podem ser aposentadas.** Depois do ajuste de denoise elas
continuam devolvendo um desenho palido: o T2I-Adapter e fraco demais para segurar a
estrutura quando a ancora de img2img sai. Custam quase o mesmo que as `cnet` e entregam
bem menos. Ficam no repositorio, mas nao sao a rota recomendada.

### O ajuste que mudou tudo, e por que

Os quatro templates chegaram com `denoise 0.75` e devolviam o rabisco redesenhado, nao uma
ilustracao. A causa nao era o modelo nem o cfg: o **mesmo desenho condicionava por dois
caminhos ao mesmo tempo**. `KSampler.latent_image` vinha de `VAEEncode(referencia)`, ou seja
img2img partindo do traco branco, e `KSampler.positive` passava por `ControlNetApply` com
`strength 1.0` sobre a mesma imagem. Os dois puxavam para o mesmo lugar e nao sobrava margem
para o prompt.

A varredura de um parametro por vez, sobre o `sdxl_lightning_t2i` com seed fixa, mostrou:

| Mudanca | Efeito |
| --- | --- |
| `denoise` 0.75 -> 0.88 | nada; ainda desenho de linha (tinta 3,4%) |
| `denoise` 0.88 -> 0.90 | **transicao abrupta**: passa a pintar (tinta 0,1%) |
| `strength` 1.0 -> 0.3 | piora; fica mais chapado (cor 1,93 -> 0,49) |
| `cfg` 1.5 -> 4.0 | quase nada |
| `scheduler` sgm_uniform -> karras/normal | quase nada |

O botao era o `denoise`, nao a forca do ControlNet - o contrario do que a intuicao sugere.
Baixar o ControlNet nao liberta o modelo porque quem prende e o latente. **Os quatro
templates foram atualizados para `denoise 0.95`**, e as copias em `toonz/build/Release` e
`toonz/build/Debug` foram sincronizadas.

Um segundo achado so apareceu ao testar o denoise nos outros templates: com `denoise 0.95`,
as variantes `cnet` melhoram muito e as `t2i` nao. E uma interacao entre os dois fatores,
invisivel numa varredura de um parametro so - por isso a recomendacao acima e por rota, e
nao um valor unico para todos.

### Correcoes a este documento

- **Secao 3 estava errada** ao dizer que somar ControlNet garantiria offload. Nao garante.
  O `sdxl_lightning_cnet` teve o **pico mais alto de todos** (7715 MiB, a 408 MiB do teto) e
  mesmo assim o tempo escalou normalmente. O offload so aparece no `sdxl_base_cnet`, ou seja
  ControlNet **combinado com** 28 passos: 36,0 s medidos contra 26,3 s previstos por um modelo
  linear ajustado nas linhas que nao saturam a placa - 9,7 s de excesso, +37%.
- **Secao 2 estava certa no essencial.** O delta medido do modelo ficou entre 5,4 e 6,0 GB,
  proximo da estimativa de ~6,8 GB de pesos e longe da estimativa inflada de 9,5 GB que a
  propria secao ja corrigia.
- **O roteiro da secao 5 tinha um defeito de metodo**, corrigido e documentado na nota
  logo abaixo da tabela: com a mesma seed o ComfyUI devolve do cache e nao mede nada.

### Onde os pesos ficaram

Fora da arvore gerenciada pelo Acer Drawing Assistant, em `C:\AI\comfy-models`, expostos por
`C:\ProgramData\CreatorSpaceFramework\ComfyUI\ComfyUI\extra_model_paths.yaml`. Se uma
atualizacao do `CreatorSpaceWindowsService` limpar a instalacao, o que se perde e esse yaml de
~1 KB, nao 9,5 GB. O cabecalho do proprio arquivo explica como recria-lo. O yaml so e lido no
arranque do processo: depois de recria-lo, e preciso reiniciar o ComfyUI.
