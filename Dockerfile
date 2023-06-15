FROM python:3.9

# Install deps
RUN apt-get update && apt-get -y install wget git locales chrpath cpio diffstat gawk zstd liblz4-tool

# Set the locale
RUN sed -i '/en_US.UTF-8/s/^# //g' /etc/locale.gen && locale-gen
ENV LANG en_US.UTF-8   
ENV LC_ALL en_US.UTF-8

# Set path names for volume and build dirs
ARG build_folder_name=build_vm_image/

ENV PROJECT_PATH /project/
ENV BUILD_PATH $PROJECT_PATH/$build_folder_name
RUN mkdir -p $BUILD_PATH


WORKDIR $PROJECT_PATH
# Copy configs and scripts
ADD ./scripts $PROJECT_PATH/scripts 
ADD ./yocto_files $PROJECT_PATH/yocto_files

CMD bash $PROJECT_PATH/scripts/prepare_yocto.sh $BUILD_PATH