# Agente de implementação — Abort e reinício de transações STM em OpenMP

## Estado e precedência

Este arquivo especifica trabalho futuro; não autoriza sua execução fora da tarefa
solicitada. O código atual só faz rastreamento. Para a política inicial, aplicar
as restrições de dados e efeitos de OPENMP_TRANSACTION_HASH_AGENT.md.
Consultar [OPENMP_TRANSACTION_ACCESS_AGENT.md](OPENMP_TRANSACTION_ACCESS_AGENT.md)
para os contratos propostos de leitura/escrita. A hash mantém registros e
referências; as primitivas de acesso controlam reservas e dados privados; este
agente rege a propagação da falha, cleanups, descarte e reinício.
Assinaturas stm_* abaixo são pseudocódigo, sem ABI aprovada; newtm_read/write
já têm esqueletos internos declarados, mas seus contratos funcionais permanecem
propostos. A definição de
propagação segura de conflito entre funções permanece pendente.

## Papel

Atue como agente de engenharia de compiladores e runtime, responsável por implementar o tratamento de falha de commit (*abort*) e a repetição de tentativas da diretiva experimental `#pragma omp transaction(...)` no GCC e na `libgomp`. Preserve a semântica abaixo. Não acrescente uma diretiva `abort` explícita nesta etapa.

## Escopo e decisões consolidadas

1. Uma transação é delimitada lexicalmente pelo bloco estruturado associado a `transaction`; o `{` não corresponde necessariamente a uma instrução ou endereço de máquina.
2. A primeira diretiva `transaction` encontrada dinamicamente por uma thread inicia a transação efetiva (*outermost*). Diretivas subsequentes, inclusive em outras funções, participam da mesma transação: **flat nesting**.
3. A lista de variáveis de cada diretiva adiciona endereços/objetos ao conjunto transacional efetivo. Uma transação interna não cria commit independente.
4. O encerramento normal de uma diretiva interna apenas conclui sua participação. Somente o encerramento da diretiva externa tenta validar e publicar as escritas.
5. Se o commit externo falhar, o runtime descarta as escritas especulativas, restaura seu estado transacional e o código gerado reinicia **todo o bloco externo**, incluindo as chamadas a funções internas. Não se reinicia isoladamente o bloco interno.
6. Não há abort explícito nem transferência assíncrona de controle. Impedir o acesso conflitante antes de executá-lo e não continuar normalmente com valores inválidos. A falha pode ser propagada até o epílogo externo somente por um caminho seguro que impeça acessos e efeitos posteriores indevidos. O mecanismo de propagação entre funções ainda precisa ser definido; restringir casos sem mecanismo seguro. **Não utilizar `setjmp`/`longjmp` para o caminho normal de retry**.
7. O estado transacional e a profundidade são locais à thread **sob a restrição de ausência de pontos de escalonamento de tarefas durante toda a transação**. Estruturas de coordenação (como uma tabela hash global de metadados) podem ser compartilhadas, com sincronização adequada.
8. Uma região `transaction` executa integralmente em uma única tarefa OpenMP, sem pontos de escalonamento de tarefas durante sua execução dinâmica. Transações aninhadas na mesma tarefa participam da transação externa. Não exigir `tied` como condição adicional: a ausência de suspensão/migração durante a transação é a propriedade decisiva.

## Transformação conceitual no compilador

Entrada:

```c
#pragma omp transaction(a, b)
{
    a++;
    b += a;
}
```

Saída conceitual (pseudocódigo, não uma ABI definitiva):

```c
retry_transaction:
    bool outermost = stm_begin(&a, &b);

    /* corpo instrumentado: verificar cada retorno de leitura/escrita;
       falhas transferem para o protocolo de descarte descrito abaixo,
       sem continuar normalmente nem confirmar no epilogo de sucesso */

    if (!stm_end(outermost)) {
        /* no modelo proposto, stm_end ja descartou a tentativa */
        goto retry_transaction;
    }
```

**Invariante fundamental:** nenhuma falha pode reiniciar apenas o bloco interno.
Após falha de acesso, impedir a continuação normal e propagar a causa para o
controle da tentativa externa por caminho seguro. Isso não significa executar
o restante do corpo até uma falha de commit: é proibido consumir resultado
inválido ou produzir efeitos posteriores indevidos. Falha permanente não é
conflito repetível. Rejeitar casos sem mecanismo seguro de propagação.

### API ilustrativa do runtime

- `stm_begin(lista_de_enderecos) -> outermost`: se não houver transação ativa, inicializa estado e retorna verdadeiro; caso contrário, incrementa profundidade, registra novas variáveis e retorna falso.
- `stm_read` / `stm_write`: nomes ilustrativos das operações detalhadas como
  `newtm_read` / `newtm_write` no agente de acesso; verificar o estado antes de
  consumir o resultado ou continuar com efeitos do programa.
- `stm_end(outermost) -> bool`: para diretivas internas, apenas reduz a profundidade e retorna verdadeiro; para a externa, valida e tenta publicar atomicamente, retornando verdadeiro em sucesso e falso em conflito após descartar a tentativa.
- `stm_rollback`: descarte exatamente uma vez por tentativa fracassada, com
  propriedade definida na seção de pré/pós-condições abaixo; evitar rollback duplicado.

Não usar o argumento `outermost` como autoridade isolada: o runtime deve conferir a coerência entre profundidade, estado e thread proprietária. Ao concluir ou descartar a transação externa, profundidade e estado ativo devem ser restaurados consistentemente.

## Pré e pós-condições das operações de ciclo de vida

Os nomes stm_* são conceituais. O modelo abaixo completa o contrato funcional
proposto, sem definir protótipos/exportações. A ponte void atual tem contrato de
rastreamento separado no agente da hash. Não implementar esses efeitos como
consequência da criação dos esqueletos de read/write.

### stm_begin(lista_de_enderecos) -> outermost

Pré-condições: runtime inicializado, execução proprietária válida, sem pontos de
escalonamento de tarefas; não há tentativa ou a existente está ativa e não
condenada. Lista válida segundo o contrato de registro, objetos vivos e tipos
suportados. Profundidade e contadores podem crescer sem overflow.

Pós-condições em sucesso: se externo, contexto/tentativa novos, profundidade um,
retorno true; se interno, mesma identidade e dados privados, profundidade +1,
retorno false. Objetos registrados sem leitura antecipada nem reserva. Duplicatas
são idempotentes e listas internas ampliam o conjunto a partir dessa entrada.
Nenhuma publicação, reinicialização da tentativa externa ou herança por outra
thread é permitida.

Falha: não publicar entrada/nivel parcialmente inicializado. O bool ilustrativo
não codifica erro — false significa aninhamento, não falha. Definir canal
separado ou erro fatal para falha permanente; alocação segue o contrato fatal da
libgomp. Contexto condenado segue descarte, não begin interno normal.

### stm_end(outermost) -> bool

Pré-condições: entrada correspondente ainda ativa, proprietário correto,
profundidade positiva e término normal de região; argumento coerente com estado
runtime. Não chamar esse caminho depois de abort que já desfez o nível. Falhas
de acesso já sinalizadas desviam para descarte; não executar o restante do corpo
para alcançar end. Cleanups que precisem da visão ativa devem ter protocolo e
ordem definidos antes de sua invalidação.

Pós-condições internas: retorno true, profundidade -1, identidade, reservas,
registros e valores privados preservados; nenhum commit independente. Tentativa
condenada não pode retornar true como se estivesse saudável.

Pós-condições externas em sucesso: retorno true somente depois de publicar todas
as escritas com reservas mantidas e depois retirar todas as participações e
referências, liberar recursos e desativar contexto/profundidade. A execução não
pode observar publicação parcial pelo protocolo.

No modelo ilustrativo de retorno false por falha recuperável antes da publicação,
end é responsável por conduzir o descarte completo antes de retornar false. O
chamador reinicia a região externa, sem chamar rollback uma segunda vez. Falha
permanente usa tratamento separado; false não a converte em conflito. Na política
de reservas persistentes, conflitos são detectados nos acessos; não inventar uma
revalidação por versões ou uma falha de commit obrigatória se não forem necessárias.
Depois da primeira publicação, não retornar false nem tentar rollback.

Este modelo de propriedade do descarte é uma proposta para eliminar ambiguidade,
não ABI consolidada. Se a implementação escolher end que só sinaliza falha,
revisar este contrato e todos os chamadores antes de integrar; não misturar modelos.

### stm_rollback: operação de descarte

Pré-condições: contexto válido e proprietário, tentativa ainda não publicada,
condenada ou encaminhada explicitamente para descarte. Nenhum acesso concorrente
usa seus buffers. Executar os cleanups suportados enquanto os recursos requeridos
continuam válidos, antes de descartá-los. Não usar rollback após commit ou após
outro descarte da mesma tentativa.

Pós-condições: nenhuma escrita especulativa publicada; reservas/participações
retiradas, referências/buffers liberados exatamente uma vez, todos os níveis
internos desfeitos e contexto inativo/profundidade zero. Valores originais não
precisam ser restaurados pois não foram escritos; efeitos externos não são
revertidos. Preservar a causa para escolher retry ou erro permanente.

O dono do descarte é o controle externo: chama rollback após falha de acesso;
no modelo end-false acima, end conduz a operação e o chamador não a repete.
Rollback não reinicia código nem espera. Duplo descarte é violação, não uma
idempotência garantida. Falha estrutural na limpeza não autoriza publicar,
continuar com estado inconsistente ou repetir indefinidamente.

### Confirmação e backoff sem novos símbolos obrigatórios

Confirmação: exige tentativa saudável, recursos preparados e reservas compatíveis
mantidas para todos os objetos; nenhuma operação falível após a primeira escrita
publicada. Ao concluir, todas as escritas são visíveis pelo protocolo antes de
liberar qualquer reserva. Falha recuperável só é possível antes da publicação.
Essa é uma fase de end, não a exigência de uma nova função com ABI já escolhida.

Backoff: exige descarte concluído, nenhuma reserva/mutex retido e nenhuma referência
a buffers invalidados. Ao retornar, não modifica dados do programa nem reativa o
contexto: apenas permite nova tentativa. Contadores e duração usam aritmética
limitada, espera progressiva com teto por espera e tentativas sem limite conforme
a política. Não promete justiça ou conclusão; falha permanente não entra nesse
caminho. Parâmetros temporais, jitter e eventual assinatura continuam pendentes.

## Estados de acesso e passagem para descarte

Usar o contrato proposto do agente de acesso sem duplicar suas assinaturas:

| Retorno | Ação do chamador e do controle transacional |
| --- | --- |
| `NEWTM_ACCESS_OK` | Consumir o resultado e continuar a tentativa ativa. |
| `NEWTM_ACCESS_CONFLICT` | Não consumir resultado; a tentativa inteira está condenada. Propagar a falha e conduzir descarte antes do retry externo. |
| `NEWTM_ACCESS_INVALID` | Falha permanente detectada; impedir commit, não consumir resultado nem iniciar retry automático. Encaminhar para diagnóstico/erro definido. |

A primitiva não deve liberar globalmente reservas, buffers e referências ao
retornar falha. Para contexto válido e proprietário correto, registrar a causa
sem perder o motivo permanente. Não acessar contexto inválido/de outra execução
para tentar condená-lo. A hash não implementa essa máquina de estados.

Sequência de tratamento de uma tentativa condenada:

1. Interromper o caminho normal no primeiro retorno de falha. Preservar a causa;
   recusar novos acessos normais e impedir qualquer confirmação da tentativa.
2. Propagar para o controle externo, inclusive através de chamadas preparadas,
   executando os cleanups suportados enquanto contexto, reservas e dados privados
   necessários ainda estão válidos. Não usar o caminho normal de acesso em
   contexto condenado como solução implícita para cleanups.
3. Descartar os valores especulativos, retirar reservas/participações e liberar
   referências/buffers conforme sua propriedade, exatamente uma vez por tentativa.
   Desfazer a profundidade inteira sem executar depois epílogos normais de níveis
   internos já desfeitos.
4. Em CONFLICT, aguardar sem reservas/locks e reiniciar todo o bloco externo,
   recriando temporários e processando novamente as listas dinâmicas.
5. Em INVALID ou outra falha permanente, não repetir. Concluir o tratamento de
   erro explícito previsto; erros fatais não precisam fingir retorno recuperável.

O mecanismo entre funções e o protocolo especial de acesso por cleanups ainda
precisam ser definidos. Restringir casos sem suporte, sem inventar convenção de
retorno de valores fictícios ou usar setjmp/longjmp no retry normal. Não converter
exceção do usuário em CONFLICT.

O TRY_FINALLY atual pertence ao rastreamento. Na integração funcional, separar
commit no término normal, descarte no desenrolamento e decisão de reinício.
Nunca confirmar automaticamente no finally de uma tentativa condenada.

## Semântica dos dados e efeitos colaterais

**Garantia restrita aos dados transacionais:** o commit bem-sucedido deve tornar visíveis as escritas transacionais como uma unidade, segundo o protocolo de isolamento e validação escolhido. Uma tentativa abortada não pode deixar alterações visíveis nesses objetos. Leituras devem observar as próprias escritas especulativas (*read-your-writes*).

**Dados não transacionais:** aplicar a política restritiva do agente da hash:
escritas em objetos cuja vida começou antes da transação externa exigem registro
prévio na lista externa ou em uma lista interna já processada. Não ampliar o
conjunto implicitamente. Restringir E/S e chamadas com efeitos externos que não
possam ser descartados ou repetidos com segurança; diagnosticar os casos
identificáveis. Esses efeitos não são revertidos pelo protocolo e a ausência de
diagnóstico não constitui suporte. Leituras externas não listadas continuam uma
decisão em aberto; não prometer consistência para elas.

Variáveis locais automáticas merecem análise separada: ao reiniciar o bloco via salto gerado, o compilador deve preservar a semântica de C/C++ e a inicialização/vida útil dos objetos. Não pressupor que `goto retry` reverta variáveis locais modificadas na tentativa anterior. Para C++, considerar construtores, destrutores e exceções; não emitir saltos que violem regras de inicialização ou destruam incorretamente objetos.

**Adição tardia de variáveis:** quando uma transação interna acrescenta um objeto ao conjunto transacional, isso não torna retroativamente transacionais acessos anteriores ao mesmo objeto. Definir uma política explícita: proibir tais acessos, diagnosticá-los quando possível, ou declarar que estão fora da garantia. Não fazer correções silenciosas.

**Concorrência:** o protocolo deve assegurar que validação e publicação não tenham janela de corrida, e que threads concorrentes não observem escritas parciais. Uma tabela hash global com sinalizadores `limpo/sujo` não basta, por si só, para assegurar isolamento, versionamento, detecção de conflitos e publicação atômica. Especificar política de conflitos, ownership/versionamento, ordem de memória e sincronização antes de declarar a implementação correta.

## Interação com tarefas OpenMP e pontos de escalonamento

**Regra semântica obrigatória:** uma região `transaction` deve executar integralmente no contexto de uma única tarefa OpenMP, **sem encontrar pontos de escalonamento de tarefas** desde a abertura da transação externa até seu encerramento. A proibição é **dinâmica e transitiva**: alcança transações internas e todas as funções chamadas durante a tentativa, inclusive código compilado separadamente. Uma tarefa pode executar `transaction` e tarefas distintas podem executar transações concorrentes; o que não se admite é suspender ou trocar de tarefa enquanto uma transação está ativa na thread.

**Proibir dentro de uma transação ativa:**

- `taskyield` e `taskwait`, incluindo variantes com dependências;
- criação/encontro de `task`, `taskloop` e demais construções geradoras de tarefas, pois podem introduzir pontos de escalonamento, ainda que uma tarefa seja executada imediatamente;
- `taskgroup`, `barrier` e pontos de espera/sincronização que introduzam escalonamento;
- `parallel` e construções de compartilhamento de trabalho (`single`, `sections`, `for`/`do` etc.) **na versão inicial**, como restrição conservadora para evitar tarefas implícitas, barreiras e caminhos de sincronização; não alegar que toda variante dessas construções necessariamente suspende a tarefa;
- cancelamento e outros caminhos que escapem ao epílogo transacional.

O conjunto acima é uma **política inicial deliberadamente conservadora**, não uma enumeração exaustiva de todos os pontos de escalonamento da especificação OpenMP. Conferir a versão da especificação adotada e abranger pontos implícitos, inclusive os introduzidos por chamadas ao runtime. Uma construção não listada também deve ser rejeitada se puder encontrar um ponto de escalonamento durante a transação.

**`tied` não basta:** a afinidade de uma tarefa `tied` à thread que a iniciou não impede sua suspensão e a execução de tarefas descendentes pela mesma thread; um TLS contendo a transação da tarefa suspensa poderia ser confundido com o contexto de outra tarefa. **Não exigir `tied` por si só**: se nenhum ponto de escalonamento for encontrado enquanto a transação está ativa, uma tarefa `untied` também não migra nesse intervalo. A regra deve ser formulada em termos de ausência de escalonamento, e não apenas de tipo da tarefa.

**`critical`, `atomic` e locks:** não são automaticamente proibidos pela regra de pontos de escalonamento, mas tampouco são automaticamente seguros. `critical` e locks podem bloquear ou causar deadlock em combinação com outras transações; `atomic`, `critical` e operações de lock que alterem estado não transacional produzem efeitos que **não são desfeitos** por rollback. Proibir ou diagnosticar o acesso a objetos transacionais por vias não instrumentadas; documentar explicitamente o tratamento de operações bloqueantes e de ordem de aquisição de locks. Na primeira versão, preferir rejeitar combinações cuja segurança não tenha sido demonstrada.

**Verificação em duas camadas:**

1. **GCC:** diagnosticar/rejeitar construções reconhecidas estaticamente dentro de `transaction`, inclusive saídas que contornem o epílogo; verificar o alcance de chamadas quando houver informação suficiente, sem presumir que uma função desconhecida é segura.
2. **`libgomp`:** acrescentar verificações nos caminhos de runtime que possam gerar pontos de escalonamento, consultando o estado transacional da thread e **falhando de modo explícito antes de suspender a tarefa**. Definir um mecanismo de erro/diagnóstico; não simplesmente marcar a transação para retry, pois uma violação estrutural persistente causaria repetição infinita. Chamadas de bibliotecas externas que realizem escalonamento sem passar pelos pontos instrumentados podem exigir restrição contratual adicional.

**Efeitos colaterais não transacionais:** obedecem às restrições acima. Ausência
de pontos de escalonamento não autoriza E/S, escritas externas não listadas ou
chamadas sem suporte. Exceções e cleanups exigem contrato próprio e não devem
ser convertidos em repetição automática.

### Exemplos de aceitação e rejeição

Permitido, desde que o corpo não contenha ponto de escalonamento nem chamada que o introduza:

```c
#pragma omp task
{
    #pragma omp transaction(a)
    { a++; }
}
```

Permitido: `transaction` externa chamando função que executa `transaction` interna, ambas na mesma tarefa e sem ponto de escalonamento.

Rejeitar:

```c
#pragma omp transaction(a)
{
    a++;
    #pragma omp taskyield
}
```

Rejeitar mesmo que a função esteja em outra unidade de tradução (diagnóstico estático quando possível, senão detecção em runtime):

```c
void auxiliar(void) { /* pode chamar omp_taskyield() ou encontrar taskwait */ }
#pragma omp transaction(a)
{
    a++;
    auxiliar();
}
```

## Restrições iniciais de controle de fluxo

- O reinício é feito por fluxo de controle gerado no **bloco externo**, não por endereço obtido do caractere `{`.
- Transações aninhadas em outras funções são suportadas se todas as chamadas executarem na mesma thread e retornarem normalmente até o commit externo.
- Não permitir que um `return`, `goto` para fora do bloco, `break`/`continue` que escape à região, `longjmp`, exceção C++ ou cancelamento OpenMP pule silenciosamente o epílogo transacional. Implementar tratamento explícito ou emitir diagnóstico/rejeitar a construção na primeira versão.
- Proibir pontos de escalonamento de tarefas durante toda a transação ativa, inclusive em chamadas indiretas; essa restrição torna o estado TLS apropriado durante a tentativa, sem exigir que a tarefa seja `tied`.
- Não fazer retry infinito de falhas permanentes sem política documentada de limitação, backoff ou erro.

## Plano de implementação

1. Identificar o parsing e a representação intermediária apropriados para a nova diretiva GCC; representar a lista de variáveis e o corpo estruturado.
2. Implementar lowering com início, epílogo e aresta de retry. Assegurar que o rótulo de retry pertença à região externa dinâmica e que o compilador não gere salto de retry para um bloco interno.
3. Implementar estado TLS: ativo, profundidade, conjuntos de leitura/escrita, metadados de validação e estado de falha.
4. Instrumentar leituras/escritas conforme o agente de acesso, preservando
   identidade e remapeamento OpenMP. Verificar cada retorno antes do uso do
   resultado; propagar CONFLICT e INVALID por caminhos distintos. Manter
   escritas privadas e reservas conforme a política do agente da hash.
5. Implementar validação, publicação atômica e descarte no runtime; documentar a linearização do commit.
6. Tratar inclusão dinâmica de variáveis por transações internas e garantir que o commit interno não publique alterações.
7. Implementar diagnósticos no GCC e verificações na `libgomp` para pontos de escalonamento de tarefas, inclusive em funções chamadas; rejeitar a violação sem permitir suspensão nem retry infinito.
8. Documentar a semântica limitada dos efeitos colaterais não transacionais e as restrições de controle de fluxo.
9. Adicionar testes de compilação, execução concorrente, aninhamento entre funções, conflito e reinício.

## Testes mínimos de aceitação

- Transação isolada: commit bem-sucedido exatamente uma vez.
- Conflito entre duas threads: ao menos uma tentativa é repetida; resultado final dos objetos transacionais equivale a uma ordem serial válida para os casos suportados.
- Aninhamento na mesma função: apenas a diretiva externa publica.
- Aninhamento em outra função: falha no commit externo reinicia a chamada à função interna e todo o bloco externo.
- Função com `transaction` chamada isoladamente: sua diretiva torna-se externa dinamicamente e faz commit próprio.
- Inclusão de nova variável em diretiva interna: o objeto participa do commit externo.
- Efeito não transacional incompatível: rejeição/diagnóstico conforme o subconjunto definido; nunca afirmar rollback desse efeito.
- Saídas antecipadas e exceções: rejeição clara ou epílogo correto, nunca vazamento silencioso de transação ativa.
- `transaction` em tarefa `tied` e `untied` sem pontos de escalonamento: execução correta, inclusive com outras tarefas transacionando em paralelo.
- `taskyield`, `taskwait`, geração de `task`/`taskloop`, `taskgroup`, barreiras e construções conservadoramente proibidas dentro de transação: diagnóstico ou erro explícito antes de qualquer suspensão.
- Função chamada (inclusive compilada separadamente) que alcança ponto de escalonamento: detecção em runtime onde houver interceptação, sem corromper TLS nem reiniciar indefinidamente.
- `critical`, `atomic` e locks: testar apenas combinações explicitamente suportadas; demonstrar que efeitos não transacionais não são revertidos e detectar combinações proibidas.
- Falha de leitura: destino preservado e nenhum consumo posterior de resultado
  inválido; falha de escrita: nenhuma aplicação parcial da alteração solicitada.
- INVALID: erro permanente sem retry infinito nem mascaramento como CONFLICT.
- Cleanups suportados veem contexto ainda válido; descarte e liberação ocorrem
  exatamente uma vez, sem epílogo normal residual de região interna desfeita.
- Após abort/commit: profundidade zero, metadados consistentes, nenhum lock ou estado especulativo abandonado.

## Critérios de entrega

Produzir alterações de código, testes reproduzíveis e documentação das decisões semânticas. Distinguir explicitamente garantias implementadas de limitações. Não acrescentar diretiva de abort explícito, não implementar nested commit independente e não utilizar `setjmp`/`longjmp` sem uma nova decisão de projeto justificada.

