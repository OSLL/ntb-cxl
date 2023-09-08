FROM ubuntu:20.04

# Install deps
RUN apt-get update && apt-get -y install wget git locales chrpath cpio diffstat gawk zstd liblz4-tool python3.8 python3-pip

# Set the locale
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen
ENV LANG en_US.UTF-8   
ENV LC_ALL en_US.UTF-8

# ID for new user
ARG user_id=1000

ENV BUILD_DIRNAME build_dir
ENV PROJECT_PATH /home/user/project
ENV BUILD_PATH $PROJECT_PATH/$BUILD_DIRNAME

# Create user for yocto
RUN useradd -rm -d /home/user -s /bin/bash -g root -G sudo user -u $user_id
USER user

# Creating build path
RUN mkdir -p $BUILD_PATH
WORKDIR $PROJECT_PATH

# Copy configs and scripts
ADD ./scripts $PROJECT_PATH/scripts 
ADD ./yocto_files $PROJECT_PATH/yocto_files

ENTRYPOINT ["/home/user/project/scripts/dispatch_docker_command.sh"]

CMD ["build"]
