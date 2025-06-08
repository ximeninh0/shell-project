
# Shell em C

Este projeto é uma implementação de um shell de linha de comando simples em C, desenvolvido para demonstrar conceitos fundamentais de sistemas operacionais. Ele oferece um ambiente interativo onde os usuários podem executar comandos, gerenciar processos e manipular a entrada e saída do sistema.

## Descrição

O Mini Shell replica o comportamento básico de shells padrão como o bash. Ele apresenta um prompt que exibe o diretório de trabalho atual e aguarda a entrada do usuário. O shell é capaz de analisar comandos com argumentos, executar programas externos, lidar com comandos integrados e gerenciar funcionalidades mais complexas, como pipelines e execução simultânea de processos.

## Funcionalidades

- Execução de Comandos: Executa comandos externos localizados nos diretórios especificados na variável `PATH` do shell (inicia com /bin por padrão).
- Comandos integrados:
    - `exit`: Encerra o shell.
    - `help`: Exibe uma mensagem de ajuda.
    - `path`: Gerencia a lista de diretórios onde o shell procura por comandos.
        - `path`: Mostra a lista de diretórios do `PATH` atual.
        - `path` + /novo/caminho: Adiciona um novo diretório à lista do `PATH`.
        - `path` - /caminho/a/remover: Remove um diretório da lista do `PATH`.
- Execução de múltiplos comandos simultaneamente usando `&`
- Redirecionamento de saída com `>` ex: `ls > saida.txt`
- Suporte a pipelines usando `|` ex: `ls -l | grep .c`
- Leitura de comandos a partir da entrada padrão

## Requisitos

- Sistema Linux
- GCC
- Bibliotecas padrão de C (stdio, stdlib, unistd, etc.)

## Compilação

Certifique-se de ter o GCC instalado em seu sistema. Para compilar o shell, execute o seguinte comando:

```bash
gcc -o shell shell.c
```

## Execução

Após a compilação, execute o shell com:

```bash
./shell
```

Após a execução, o prompt do shell será exibido:
```bash
========== AJUDA DO SHELL ==========
Comandos suportados:
 - exit: Sai do shell.
 - cd <diretorio>: Muda o diretório atual.
 - pwd: Mostra o diretório atual.
 - path [+ /novo/caminho] [- /caminho/a/remover]: Gerencia os diretórios onde procurar comandos.
 - ls: Lista arquivos no diretório atual.
 - cat <arquivo>: Exibe o conteúdo de um arquivo.
 - Redirecionamento de saída com > é suportado.
 - Comandos podem ser encadeados com & para execução simultânea.
 - Comandos podem ser encadeados com | para execução em pipeline.
 - Argumentos devem ser separados por espaços, ex: ls -a -l.
 - Para executar outros programas, use o caminho completo ou defina o PATH com o comando path.
====================================
MINI SHELL:/caminho/atual $:
```
## Exemplos de Uso

- Executar um comando simples:
  ```bash
  ls -l
  ```

- Executar comandos em paralelo:
  ```bash
  ls & pwd
  ```

- Redirecionar saída para um arquivo:
  ```bash
  ls > arquivos.txt
  ```

- Usar pipeline:
  ```bash
  ls | grep "main"
  ```

## Estrutura do Código

| Função | Descrição |
|-------------|-------------|
| `main`  | O ponto de entrada principal. Contém o loop que lê a entrada do usuário e chama o processador de comandos.  |
| `process_command_line` | Analisa se a entrada é um comando simples, um built-in ou um pipeline e direciona para a função de execução apropriada.  |
| `simultaneos_proc` | Divide a linha de entrada com base no caractere `&` para identificar comandos que devem ser executados simultaneamente.|
| `split_pipeline_args` | Divide um grupo de comandos com base no caractere |
| `execute` | Executa um único comando, lidando com o redirecionamento de saída `>` antes de criar um novo processo.| 
| `execute_pipeline` | Orquestra a execução de comandos em um pipeline, configurando os pipes (`pipe()`) para conectar a saída de um processo à entrada do próximo.|
| `launch_process` | Cria um novo processo filho com `fork()` e utiliza `execv()` para substituir a imagem do processo pelo comando a ser executado. |
| `find_and_exec_command` | Procura o executável do comando nos diretórios da lista `PATH` e o executa.|
| `handle_builtin_command` | Verifica se um comando é um dos comandos integrados (`cd`, `path`, `exit`, etc.) o executa diretamente.|
| Funções de `Lista` | Um conjunto de funções (`init`, `insert`, `remove_by_value`, etc.) para gerenciar a lista encadeada que armazena os diretórios do `PATH`.|

O código é organizado com:

- Manipulação de argumentos via listas de strings
- Uso de `fork()`, `execvp()` e `wait()` para gerenciar processos
- Manipulação de arquivos com `open()`, `dup2()` para redirecionamento
- Tratamento de diretórios e caminhos

## Contribuidores do Projeto
<table>
  <tr>
    <td align="center">
      <a href="https://github.com/ximeninh0">
      <img src="https://avatars.githubusercontent.com/u/178533035?v=4" width="100px;" alt="Foto de ximeninh0 no GitHub"/><br>
      <sub>
      <b>Guilherme Ximenes</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/guiandradedev">
      <img src="https://avatars.githubusercontent.com/u/42524262?v=4" width="100px;" alt="Foto de guiandradedev no GitHub"/><br>
      <sub>
      <b>Guilherme Andrade</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/LuigiZanon">
      <img src="https://avatars.githubusercontent.com/u/133017207?v=4" width="100px;" alt="Foto de LuigiZanon no GitHub"/><br>
      <sub>
      <b>Luigi Zanon</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/Rahmegm">
      <img src="https://avatars.githubusercontent.com/u/142615888?v=4" width="100px;" alt="Foto de Rahmegm no GitHub"/><br>
      <sub>
      <b>Gustavo Rahmé</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/PG2zinho">
      <img src="https://avatars.githubusercontent.com/u/125620181?v=4" width="100px;" alt="Foto de PG2zinho no GitHub"/><br>
      <sub>
      <b>Pedro Gasparotto</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/isabelacrestana">
      <img src="https://avatars.githubusercontent.com/u/126805402?v=4" width="100px;" alt="Foto de isabelacrestana no GitHub"/><br>
      <sub>
      <b>Isabela Crestana</b>
      </sub>
      </a>
    </td>
    <td align="center">
      <a href="https://github.com/Campos2201">
      <img src="https://avatars.githubusercontent.com/u/101422422?v=4" width="100px;" alt="Foto de Campos2201 no GitHub"/><br>
      <sub>
      <b>Gustavo Campos</b>
      </sub>
      </a>
    </td>
  </tr>
</table>

## Licença

Este projeto é de uso livre para fins educacionais.

---
Desenvolvido como parte de um estudo sobre interpretação de comandos  gerenciamento de processos em sistemas Unix-like.
