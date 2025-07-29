
# Datalogger de Movimento com IMU 🏃‍♂️

**Datalogger de Movimento com IMU** é um sistema embarcado para captura e registro de dados de movimento, construído sobre a  **Raspberry Pi Pico** . O projeto utiliza um sensor IMU (Unidade de Medição Inercial) para coletar dados de aceleração e giroscópio, armazenando-os em um cartão MicroSD para análise posterior.

Ele oferece:

* **Captura de Dados de Movimento** com o sensor IMU MPU6050 (acelerômetro de 3 eixos e giroscópio de 3 eixos).
* **Armazenamento de Dados Estruturado** em formato `.csv` em um cartão MicroSD, utilizando a biblioteca FatFs.
* **Feedback Interativo em Tempo Real** através de um display OLED, LED RGB e Buzzer para informar o status do sistema (calibrando, aguardando, gravando, erro).
* **Firmware Robusto em C/C++** utilizando o Pico SDK, com rotina de calibração de offset para maior precisão dos dados.
* **Script de Análise em Python** para ler os dados coletados e gerar gráficos de aceleração e giroscópio.

> Projeto de código aberto mantido por **Heitor Lemos** sob a Licença MIT.

## 📂 Estrutura do Repositório

| **Caminho**                | **Descrição**                                                                                                                                           |
| -------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `datalogger.c`                 | Código principal do firmware: inicializa hardware, calibra o sensor, gerencia os estados de operação (gravação, espera) e armazena os dados no cartão SD. |
| `analise_dados.py`             | Script em Python para ser executado no computador. Lê o arquivo `.csv`gerado e plota os dados de aceleração e giroscópio para análise visual.            |
| `lib/`                         | Contém os drivers para os periféricos e bibliotecas de terceiros.                                                                                             |
| `lib/ssd1306.c`·`ssd1306.h` | Driver I²C para o display OLED SSD1306.                                                                                                                        |
| `lib/ff.c`·`ff.h`           | Biblioteca FatFs, um módulo de sistema de arquivos genérico para sistemas embarcados.                                                                         |
| `lib/sd_card.c`·`sd_card.h` | Funções de baixo nível para comunicação com o cartão SD via SPI.                                                                                          |
| `CMakeLists.txt`               | Script de build e configuração do projeto para o CMake.                                                                                                       |
| `pico_sdk_import.cmake`        | Script do SDK para importação de dependências do Pico.                                                                                                       |

Exportar para as Planilhas

## 🔧 Requisitos

### Hardware

Foi utilizado a BitDogLab com os seguintes componentes conectados (ou já soldados na placa).

| **Componente**             | **Qtde** | **Observação**                                       |
| -------------------------------- | -------------- | ------------------------------------------------------------ |
| **Raspberry Pi Pico**      | 1              | Pode ser a versão normal ou a W (Versão da BitDogLab).     |
| **Sensor IMU MPU6050**     | 1              | Medição de aceleração e giroscópio (I²C).              |
| **Módulo MicroSD Card**   | 1              | Para interface com o cartão SD (SPI).                       |
| **Cartão MicroSD**        | 1              | Formatado em FAT32.                                          |
| **Display OLED SSD1306**   | 1              | Display I²C para visualização de status.                  |
| **LED RGB (Comum)**        | 1              | 3 pinos (R, G, B) para feedback visual de status.            |
| **Buzzer Passivo**         | 1              | Para feedback sonoro.                                        |
| **Push Buttons (Botões)** | 2              | Para controle de Iniciar/Parar e recuperação de erro do SD |

Exportar para as Planilhas

### Software

#### Para o Firmware (Pico)

| **Ferramenta**                 | **Versão Mínima** |
| ------------------------------------ | ------------------------- |
| **Pico SDK**                   | 1.5.0+                    |
| **CMake**                      | 3.13+                     |
| **GNU Arm Embedded GCC**       | 10.3+                     |
| **Extensão Pi Pico (VSCode)** |                           |

Exportar para as Planilhas

#### Para Análise dos Dados (Computador)

| **Ferramenta** | **Versão** | **Comando de Instalação** |
| -------------------- | ----------------- | --------------------------------- |
| **Python**     | 3.7+              | -                                 |
| **pandas**     | Qualquer          | `pip install pandas`            |
| **matplotlib** | Qualquer          | `pip install matplotlib`        |

Exportar para as Planilhas

## 📡 Diagrama do Sistema

O diagrama abaixo ilustra o fluxo de dados e controle entre os componentes do hardware:

**Snippet de código**

 ```mermaid
graph TD
    subgraph "Sensor"
        MPU6050[🌍 IMU MPU6050<br/>Acel/Giro]
    end

    subgraph "Raspberry Pi Pico"
        Pico[🤖 Pico<br/>Firmware Principal]
    end

    subgraph "Armazenamento"
        SD[💾 Cartão MicroSD<br/>Gravação .csv]
    end

    subgraph "Interface com Usuário"
        OLED[📺 Display OLED<br/>Status Visual]
        RGB[💡 LED RGB<br/>Status Colorido]
        Buzzer[🔊 Buzzer<br/>Feedback Sonoro]
        Botoes[👇 Botões<br/>Controle Manual]
    end

    MPU6050 -- I²C --> Pico
    Pico -- SPI --> SD
    Pico -- I²C --> OLED
    Pico -- GPIO --> RGB
    Pico -- GPIO --> Buzzer
    Botoes -- GPIO --> Pico
```

## ⚙️ Configuração e Compilação

### 1. Clonar o Repositório

**Bash**

```
git clone <URL_DO_SEU_REPOSITORIO>
cd <NOME_DA_PASTA>
```

### 2. Compilar o Projeto

No terminal, a partir da raiz do projeto, execute os seguintes comandos:

**Bash**

```
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### 3. Flashing para o Pico

1. Mantenha o botão **BOOTSEL** do Pico pressionado.
2. Conecte o cabo USB ao seu computador e solte o botão.
3. O Pico será montado como um disco chamado `RPI-RP2`.
4. Arraste e solte o arquivo `build/datalogger_imu.uf2` para dentro desse disco.

## ▶️ Como Usar o Datalogger

1. **Ligar e Calibrar:** Conecte a alimentação. **Mantenha o dispositivo parado e nivelado** enquanto o LED estiver **Laranja** e o display mostrar "Calibrando...".
2. **Aguardar:** Após a calibração, o LED ficará **Verde** e o display mostrará "Aguardando". O dispositivo está pronto.
3. **Gravar:** Pressione o  **Botão 1** . O LED ficará  **Vermelho** , o buzzer dará 1 beep e o display mostrará a contagem de amostras.
4. **Parar:** Pressione o **Botão 1** novamente. O LED voltará para  **Verde** , o buzzer dará 2 beeps e os dados estarão salvos no cartão.
5. **Recuperar Dados:** Com o LED Verde, desligue o aparelho e remova o cartão SD para ler no computador.

## 📊 Análise dos Dados

1. Copie o arquivo `datalog.csv` do cartão SD para a mesma pasta do script `analise_dados.py` no seu computador.
2. Certifique-se de ter Python, pandas e matplotlib instalados.
3. Abra um terminal na pasta do projeto e execute:
   **Bash**

   ```
   python analise_dados.py
   ```
4. Uma janela será exibida com os gráficos de aceleração e giroscópio.

## 🤝 Contribuindo

Contribuições são bem-vindas! Se você tiver sugestões para melhorar o projeto, sinta-se à vontade para fazer um *fork* e abrir um  *Pull Request* .

1. Faça um Fork do projeto.
2. Crie sua Feature Branch (`git checkout -b feature/MinhaFeature`).
3. Faça o Commit de suas mudanças (`git commit -m 'Adiciona MinhaFeature'`).
4. Faça o Push para a Branch (`git push origin feature/MinhaFeature`).
5. Abra um Pull Request.

## 📝 Licença

Distribuído sob a  **Licença MIT** . Veja o arquivo `LICENSE` para mais detalhes.

## 📞 Contato

**Heitor Lemos**

🔗  **GitHub** : `https://github.com/TorRLD`
