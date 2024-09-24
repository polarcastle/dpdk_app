    #!/bin/bash
    # 架构
    arch="amd64"
    # 要下载的部分（可根据需求调整）
    section="main,restricted,universe,multiverse"
    # Ubuntu 版本
    release="jammy,jammy-updates,jammy-backports"  # Ubuntu 22.04 的代号是 jammy
    # 使用的协议（http、ftp 等）
    proto="https"
    # 服务器地址
    server="mirrors.tuna.tsinghua.edu.cn/ubuntu/"
    # 镜像存储路径
    outpath="/opt/ubuntu_22.04_mirror"
    
    debmirror -a "$arch" --no-source --progress --verbose --ignore-release-gpg --i18n -s "$section" -h "$server" -d "$release" -r "$server/ubuntu" -e "$proto" "$outpath"