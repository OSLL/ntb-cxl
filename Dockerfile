FROM ubuntu:20.04

# Install deps
RUN apt-get update && apt-get -y install wget git locales chrpath cpio diffstat gawk zstd liblz4-tool python3.8 python3-pip

# Set the locale
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen
ENV LANG en_US.UTF-8   
ENV LC_ALL en_US.UTF-8

# Set path names for volume and build dirs
ARG build_folder_name=build_vm_image/
ARG user_id=1000

ENV PROJECT_PATH /home/user/project
ENV BUILD_PATH $PROJECT_PATH/$build_folder_name

# Create user for yocto
RUN useradd -rm -d /home/user -s /bin/bash -g root -G sudo user -u $user_id
USER user

# Creating build path
RUN mkdir -p $BUILD_PATH
WORKDIR $PROJECT_PATH

# Copy configs and scripts
ADD ./scripts $PROJECT_PATH/scripts 
ADD ./yocto_files $PROJECT_PATH/yocto_files

ENTRYPOINT ["scripts/dispatch_docker_command.sh"]

CMD ["build"]
