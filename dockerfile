# Usando a versão LTS estável do Ubuntu
FROM ubuntu:22.04

# Evita que o instalador do Ubuntu fique travado pedindo fuso horário ou idioma
ENV DEBIAN_FRONTEND=noninteractive

# Atualiza os repositórios e instala as dependências
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    gdb \
    libgdal-dev \
    gdal-bin \
    && rm -rf /var/lib/apt/lists/*

# Define o diretório de trabalho padrão ao entrar no container
WORKDIR /app

# Ao rodar o container, abre o terminal bash por padrão
CMD ["/bin/bash"]