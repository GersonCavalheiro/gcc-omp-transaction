# Exemplos da diretiva experimental transaction

Execute neste diretorio:

```sh
make check
```

O Makefile usa o GCC em `/usr/src/gcc-build` pela localizacao relativa desta
arvore. Para outro build, use `make check GCC_BUILD=/caminho/do/build`.
O build deve conter C e a libgomp com os simbolos `GOMP_transaction_*`.
Os executaveis e logs ficam em `.build/`; o antigo `a.out` nao e utilizado.
Recompile os executaveis ao mudar de compilador ou flags, por exemplo:
`make -B check CFLAGS='-O0 -Wall -Wextra'`.

- `transaction_exemplo.c`: procura o maximo em paralelo, compara valor e posicao
  com uma referencia sequencial e testa um empate proposital. Lista os objetos
  efetivamente alterados (`maior` e `posicao`). Um `critical` externo sincroniza
  a atualizacao, pois transaction ainda nao oferece isolamento.
- `transaction_aninhamento.c`: executa uma regiao interna lexical e outra em
  funcao chamada. Exercita argumento repetido, reutilizacao do objeto externo e
  inclusao de um novo objeto na lista interna. Os objetos permanecem vivos ate
  a saida externa. Nao ha concorrencia neste caso.
- `verificar.py`: verifica o resultado do maximo com 1, 2 e 4 threads solicitadas,
  as contagens dos logs e, no exemplo aninhado, a ausencia de liberacao nas saidas
  internas. Aceita equipes menores impostas pelo ambiente e relata seu tamanho.

Estes exemplos verificam o registro e a vida util da ponte de rastreamento.
A hash continua sem operacoes reais. Eles nao comprovam atomicidade, isolamento,
commit, abort ou reinicio. Os logs e sua verificacao precisarao ser revistos
quando a hash e a STM forem implementadas. Os `puts` internos do exemplo
aninhado servem apenas a esta etapa; nao representam E/S aprovada para uma
transacao com repeticao. Testes negativos de sintaxe, escopo e categorias de
identificadores continuam na suite DejaGNU do GCC (`transaction-*`).
