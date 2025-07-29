import pandas as pd
import matplotlib.pyplot as plt
import os

# --- CONFIGURAÇÕES ---
# Nome do arquivo CSV gerado pelo datalogger
NOME_ARQUIVO_CSV = 'datalog.csv'

def plotar_dados(dataframe):
    """
    Plota os dados de aceleração e giroscópio em gráficos separados.
    
    Args:
        dataframe (pd.DataFrame): O DataFrame contendo os dados do sensor.
    """
    if dataframe.empty:
        print("O arquivo de dados está vazio. Nada para plotar.")
        return

    # Garante que o índice (número da amostra) seja usado como eixo X
    eixo_x = dataframe['numero_amostra']

    # Cria a figura e os subplots
    # 2 linhas, 1 coluna, tamanho da figura ajustado para boa visualização
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 10), sharex=True)
    
    # --- GRÁFICO DO ACELERÔMETRO ---
    ax1.plot(eixo_x, dataframe['accel_x'], label='Accel X', color='r')
    ax1.plot(eixo_x, dataframe['accel_y'], label='Accel Y', color='g')
    ax1.plot(eixo_x, dataframe['accel_z'], label='Accel Z', color='b')
    ax1.set_title('Dados do Acelerômetro', fontsize=16)
    ax1.set_ylabel('Valor Raw do Sensor')
    ax1.legend()
    ax1.grid(True, linestyle='--', alpha=0.6)

    # --- GRÁFICO DO GIROSCÓPIO ---
    ax2.plot(eixo_x, dataframe['giro_x'], label='Giro X', color='c')
    ax2.plot(eixo_x, dataframe['giro_y'], label='Giro Y', color='m')
    ax2.plot(eixo_x, dataframe['giro_z'], label='Giro Z', color='y')
    ax2.set_title('Dados do Giroscópio', fontsize=16)
    ax2.set_xlabel('Número da Amostra (Tempo)', fontsize=12)
    ax2.set_ylabel('Valor Raw do Sensor')
    ax2.legend()
    ax2.grid(True, linestyle='--', alpha=0.6)

    # Ajusta o layout para evitar sobreposição de títulos e eixos
    plt.tight_layout()
    
    # Exibe os gráficos
    plt.show()

def main():
    """
    Função principal que carrega os dados e chama a função de plotagem.
    """
    print(f"Tentando carregar os dados do arquivo: '{NOME_ARQUIVO_CSV}'")
    
    # Verifica se o arquivo existe no diretório atual
    if not os.path.exists(NOME_ARQUIVO_CSV):
        print(f"\nERRO: O arquivo '{NOME_ARQUIVO_CSV}' não foi encontrado.")
        print("Verifique se o arquivo está no mesmo diretório que o script,")
        print("ou especifique o caminho completo para o arquivo.")
        return

    try:
        # Carrega o arquivo CSV usando pandas
        dados_df = pd.read_csv(NOME_ARQUIVO_CSV)
        
        # Verifica se as colunas esperadas existem
        colunas_necessarias = ['numero_amostra', 'accel_x', 'accel_y', 'accel_z', 'giro_x', 'giro_y', 'giro_z']
        if not all(coluna in dados_df.columns for coluna in colunas_necessarias):
            print("\nERRO: O arquivo CSV não contém as colunas esperadas.")
            print(f"Esperado: {colunas_necessarias}")
            print(f"Encontrado: {list(dados_df.columns)}")
            return
            
        print("Arquivo carregado com sucesso!")
        print(f"Total de {len(dados_df)} amostras encontradas.")
        
        # Chama a função para gerar os gráficos
        plotar_dados(dados_df)

    except Exception as e:
        print(f"\nOcorreu um erro ao processar o arquivo: {e}")

if __name__ == '__main__':
    main()