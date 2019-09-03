



# Step-by-Step 快速上手 AWS IoT OTA 固件升级



为了保证物联网设备能够保持在功能上随时更新，并且在出现问题的时候及时得到修复。小到智能手环，空气净化器，大到家用汽车，设备厂商无不是通过提供OTA（Over-The-Air）功能来提高用户满意度。而利用 AWS IoT Device Management 中的 Jobs 组件，可以帮助客户非常快速的开发出物联网设备的 OTA 功能。本文旨在帮助读者一步步的快速上手并理解 OTA 升级流程，其中会使用到 AWS IoT Core, IoT Device Management, EC2 以及 S3 的相关功能。关于在开发过程中的具体流程可以配合参考 [AWS IoT Device SDK](https://docs.aws.amazon.com/iot/latest/developerguide/iot-sdks.html) 文档和另外一篇[官方博客](https://aws.amazon.com/cn/blogs/china/aws-iot-series-4/)。



## 准备工作

1. 使用具有 admin 权限的用户登陆 AWS Console。

2. 本文中的AWS IoT 设备会使用一台[Amazon Linux EC2](https://aws.amazon.com/cn/ec2/)实例模拟，Amazon Linux EC2 实例上默认安装了AWS 命令行工具AWSCLI，接下来的所有操作都是以 AWS 北京区为示例。启动一台 Amazon Linux EC2 实例作为模拟的IoT设备，由于后面安装要安装的rpm包有依赖关系，这里要确保使用的是 Amazon Linux AMI 2018.03.0 (HVM)。另外为了保证网络畅通，在实验环节Security Group建议开放全部的IP和端口。
3. 在 AWS Console 上赋予这台 EC2 实例一个具有足够权限的 Role，测试中可以直接用admin权限。

4. 登陆到 EC2 实例上使用 aws configure 命令配置好默认 Region 为 cn-north-1。



## 操作流程

1. 环境准备

2. 在 AWS 上创建 IoT Thing

3. 编写 AWS IoT Jobs 文档

4. 运行 IoT 设备端程序

5. 创建 AWS IoT Jobs 进行固件升级

6. 验证固件升级是否成功



## 第一步 - 环境准备

1. 登陆 EC2 实例，安装 git

```shell
$ sudo yum install git
```

2. 安装 node.js

```shell
$ curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.34.0/install.sh | bash
$ . ~/.nvm/nvm.sh
$ nvm install node
```

3. 安装 AWS IoT Device SDK - Javascript

```shell
$ git clone https://github.com/aws/aws-iot-device-sdk-js.git
$ cd aws-iot-device-sdk-js
$ npm install
```

4. 下载两个不同版本的 telnet 程序包，后续模拟固件升级时使用

```shell
$ cd ~/aws-iot-device-sdk-js/examples/
$ wget https://www.rpmfind.net/linux/centos/6.10/os/x86_64/Packages/telnet-0.17-48.el6.x86_64.rpm
$ wget https://www.rpmfind.net/linux/centos/7.6.1810/os/x86_64/Packages/telnet-0.17-64.el7.x86_64.rpm
```

5. 安装旧版本 telnet 程序，后续我们会通过 OTA 完成这个程序从 telnet-0.17-48.el6.x86_64 版本到 telnet-0.17-64.el7.x86_64 版本的升级

```shell
$ sudo rpm -ivh telnet-0.17-48.el6.x86_64.rpm 
```

6. 创建一个 S3 bucket 作为新固件的存储位置，并上传新版本的 telnet 程序

```shell
$ aws s3 mb s3://bucket-name # 桶名请替换成自己定义的名称
$ aws s3 cp telnet-0.17-64.el7.x86_64.rpm s3://bucket-name
```



## 第二步 - 在 AWS 上创建 IOT thing

1. 创建 IoT thing，记录下输出中的 thingArn:

```shell
$ aws iot create-thing --thing-name aws-iot-device-sdk-js
{
    "thingArn": "arn:aws-cn:iot:cn-north-1:408221054609:thing/aws-iot-device-sdk-js", 
    "thingName": "aws-iot-device-sdk-js", 
    "thingId": "35e3e6ab-da11-489f-8375-196427cb61f4"
}
```

2. 下载 AWS IoT 根证书，创建 IoT 设备证书和密钥，记录下生成的 certificateArn:

```shell
$ pwd
/home/ec2-user/aws-iot-device-sdk-js/examples
$ mkdir certs
$ cd certs
$ wget https://www.amazontrust.com/repository/AmazonRootCA1.pem
$ mv AmazonRootCA1.pem root-CA.crt
$ aws iot create-keys-and-certificate \
    --certificate-pem-outfile "certificate.pem.crt" \
    --public-key-outfile "public.pem.key" \
    --private-key-outfile "private.pem.key"
```

```json
#从上一步的命令输出中记录下自己的 certificateArn, 后面的命令中会用到
#example: 

"certificateArn": "arn:aws-cn:iot:cn-north-1:408221054609:cert/661bdfb4f083bf58607ac1a54904162e0f91f542e9969b58ee10136ded565925"
```

3. 创建一个 IoT Policy，挂载给证书并激活证书:

```shell
$ cd .. 
$ pwd
/home/ec2-user/aws-iot-device-sdk-js/examples

# 编写一个 policy 文档，复制以下JSON格式的策略并保存为 iot-policy.json 文件
$ vi iot-policy.json  
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Effect": "Allow",
      "Action": [
        "iot:Publish",
        "iot:Subscribe",
        "iot:Connect",
        "iot:Receive"
      ],
      "Resource": [
        "*"
      ]
    }
  ]
}

# 创建 iot policy
$ aws iot create-policy --policy-name ota-policy --policy-document file://iot-policy.json 

# 挂载 policy 到之前创建的 IoT 设备证书上，注意这里的 --target 替换成自己的证书Arn
$ aws iot attach-policy \
    --policy-name ota-policy \
    --target "arn:aws-cn:iot:cn-north-1:408221054609:cert/661bdfb4f083bf58607ac1a54904162e0f91f542e9969b58ee10136ded565925"
    
# 激活证书，注意 --certificate-id 替换成自己证书的id
$ aws iot update-certificate --certificate-id 661bdfb4f083bf58607ac1a54904162e0f91f542e9969b58ee10136ded565925 --new-status ACTIVE

# Attach thing 到证书，其中 --principal 是自己证书的 Arn
$ aws iot attach-thing-principal --thing-name aws-iot-device-sdk-js --principal arn:aws-cn:iot:cn-north-1:408221054609:cert/661bdfb4f083bf58607ac1a54904162e0f91f542e9969b58ee10136ded565925
```



## 第三步 - 编写 AWS IoT Jobs 文档



1. 编写一个 IoT Jobs 文档。关于文档编写的格式，请参考 https://github.com/aws/aws-iot-device-sdk-js#jobsAgent。当 IoT 设备请求 IoT Jobs 文档时，AWS IoT 会生成预签名 URL 并使用预签名 URL 替换占位符 URL。然后将 IoT Jobs 文档发送到设备，设备会通过这个预签名 URL 取得访问 S3 bucket 中固件的权限。

```shell
$ pwd
/home/ec2-user/aws-iot-device-sdk-js/examples

# 编写一个jobs文档，复制以下JSON格式文档并保存为 jobs-document.json 文件。其中 “bucket-name” 请替换成您自己 S3 桶的名字。
$ vi jobs-document.json
{
  "operation": "install",
  "packageName": "new-firmware",
  "workingDirectory": "../examples",
  "launchCommand": "sudo rpm -Uvh new-firmware.rpm",
  "autoStart": "true",
  "files": [
    {
      "fileName": "new-firmware.rpm",
      "fileVersion": "1.0",
      "fileSource": {
        "url": "${aws:iot:s3-presigned-url:https://bucket-name.s3.cn-north-1.amazonaws.com.cn/telnet-0.17-64.el7.x86_64.rpm}"
      }
    }
  ]
}
```

2. 上传 IoT Jobs 文档到 S3 bucket

```shell
$ aws s3 cp jobs-document.json s3://bucket-name
```

3. 在创建使用预签名 Amazon S3 URL 的 Job 时，您必须提供一个 IAM 角色，该角色可授予 AWS IoT 服务从 Amazon S3 存储桶中下载文件的权限。该角色还必须向 AWS IoT 授予 assumeRole 的权限，也就是让 AWS IoT 具有代表设备去 S3 上面下载固件的权限。

```shell
# 编写一个 assumeRole 的 policy 文档，复制以下JSON格式的策略并保存为 trust-policy.json 文件
$ vi trust-policy.json 
{
  "Version": "2012-10-17",
  "Statement": [
    {
      "Sid": "",
      "Effect": "Allow",
      "Principal": {
        "Service": [
          "iot.amazonaws.com"
        ]
      },
      "Action": "sts:AssumeRole"
    }
  ]
}
```

```shell
# 创建 IAM Role, 记录下 Arn
$ aws iam create-role --role-name iot-access-s3 --assume-role-policy-document file://trust-policy.json 
{
    "Role": {
        "AssumeRolePolicyDocument": {
            "Version": "2012-10-17", 
            "Statement": [
                {
                    "Action": "sts:AssumeRole", 
                    "Principal": {
                        "Service": [
                            "iot.amazonaws.com"
                        ]
                    }, 
                    "Effect": "Allow", 
                    "Sid": ""
                }
            ]
        }, 
        "RoleId": "AROAV6C6662IW2BAC4NEW", 
        "CreateDate": "2019-08-27T04:58:31Z", 
        "RoleName": "iot-access-s3", 
        "Path": "/", 
        "Arn": "arn:aws-cn:iam::408221054609:role/iot-access-s3"
    }
}
```

```shell
# 编写一个从 S3 存储桶下载文件的 policy 文档，制以下JSON格式的策略并保存为 s3-policy.json 文件
$ vi s3-policy.json
{
    "Version": "2012-10-17",
    "Statement": [
        {
            "Effect": "Allow",
            "Action": "s3:GetObject",
            "Resource": "arn:aws-cn:s3:::bucket-name/*" #这里替换成自己的 bucket name
        }
    ]
}
```

```shell
# 创建 policy, 记录下 Arn
$ aws iam create-policy --policy-name iot-access-s3 --policy-document file://s3-policy.json
{
    "Policy": {
        "PolicyName": "iot-access-s3", 
        "PermissionsBoundaryUsageCount": 0, 
        "CreateDate": "2019-08-27T05:18:58Z", 
        "AttachmentCount": 0, 
        "IsAttachable": true, 
        "PolicyId": "ANPAV6C6662I6TMQS6KAL", 
        "DefaultVersionId": "v1", 
        "Path": "/", 
        "Arn": "arn:aws-cn:iam::408221054609:policy/iot-access-s3", 
        "UpdateDate": "2019-08-27T05:18:58Z"
    }
}
```

```shell
# 挂载 policy iot-access-s3 到 role iot-access-s3
$ aws iam attach-role-policy --role-name iot-access-s3 --policy-arn arn:aws-cn:iam::408221054609:policy/iot-access-s3
```



## 第四步 - 运行 IoT 设备端程序

```shell
$ pwd
/home/ec2-user/aws-iot-device-sdk-js/examples

# 查看自己的 AWS IoT Endpoint
$ aws iot describe-endpoint --endpoint-type iot:Data-ATS
{
    "endpointAddress": "a1hk0pcc0rk07l.ats.iot.cn-north-1.amazonaws.com.cn"
}

# 运行客户端程序 jobs-agent.js，并等待jobs的提交
$ node jobs-agent.js -f ~/aws-iot-device-sdk-js/examples/certs -H a1hk0pcc0rk07l.ats.iot.cn-north-1.amazonaws.com.cn -T aws-iot-device-sdk-js -D

{
  keyPath: '/home/ec2-user/aws-iot-device-sdk-js/examples/certs/private.pem.key',
  certPath: '/home/ec2-user/aws-iot-device-sdk-js/examples/certs/certificate.pem.crt',
  caPath: '/home/ec2-user/aws-iot-device-sdk-js/examples/certs/root-CA.crt',
  clientId: 'ec2-user52261',
  region: undefined,
  baseReconnectTimeMs: 4000,
  keepalive: 300,
  protocol: 'mqtts',
  port: 8883,
  host: 'a1hk0pcc0rk07l.ats.iot.cn-north-1.amazonaws.com.cn',
  thingName: 'aws-iot-device-sdk-js',
  debug: true,
  username: '?SDK=JavaScript&Version=2.2.1',
  reconnectPeriod: 4000,
  fastDisconnectDetection: true,
  resubscribe: false,
  key: <Buffer 2d 2d 2d 2d 2d 42 45 47 49 4e 20 52 53 41 20 50 52 49 56 41 54 45 20 4b 45 59 2d 2d 2d 2d 2d 0a 4d 49 49 45 6f 77 49 42 41 41 4b 43 41 51 45 41 77 76 ... 1625 more bytes>,
  cert: <Buffer 2d 2d 2d 2d 2d 42 45 47 49 4e 20 43 45 52 54 49 46 49 43 41 54 45 2d 2d 2d 2d 2d 0a 4d 49 49 44 57 54 43 43 41 6b 47 67 41 77 49 42 41 67 49 55 49 68 ... 1170 more bytes>,
  ca: <Buffer 2d 2d 2d 2d 2d 42 45 47 49 4e 20 43 45 52 54 49 46 49 43 41 54 45 2d 2d 2d 2d 2d 0a 4d 49 49 44 51 54 43 43 41 69 6d 67 41 77 49 42 41 67 49 54 42 6d ... 1138 more bytes>,
  requestCert: true,
  rejectUnauthorized: true
}
attempting new mqtt connection...
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'shutdown' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'reboot' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'install' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'systemStatus' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'stop' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'start' }
subscribeToJobs: { thingName: 'aws-iot-device-sdk-js', operationName: 'restart' }
agent connected
startJobNotifications completed for thing: aws-iot-device-sdk-js
```



## 第五步 - 创建 AWS IoT Jobs 进行固件升级

1. 新开一个命令行窗口到 EC2 实例，查看当前固件版本

```shell
$ rpm -qa |grep telnet
telnet-0.17-48.el6.x86_64
```

2. 创建 AWS IoT Jobs

```shell
$ cd /home/ec2-user/aws-iot-device-sdk-js/examples

#注意这里的 --targets, bucket-name 和 roleArn 要替换成自己的
$ aws iot create-job --job-id 1 --targets arn:aws-cn:iot:cn-north-1:408221054609:thing/aws-iot-device-sdk-js --document-source https://bucket-name.s3.cn-north-1.amazonaws.com.cn/jobs-document.json --presigned-url-config "{\"roleArn\":\"arn:aws-cn:iam::408221054609:role/iot-access-s3\", \"expiresInSec\":3600}" --target-selection SNAPSHOT
{
    "jobArn": "arn:aws-cn:iot:cn-north-1:408221054609:job/1", 
    "jobId": "1"
}
```



## 第六步 - 验证固件升级是否成功

1. 查看之前 IoT设备端程序输出

```shell
agent connected
startJobNotifications completed for thing: aws-iot-device-sdk-js
job execution handler invoked: { thingName: 'aws-iot-device-sdk-js', operationName: 'install' }
updateJobStatus: {
  thingName: 'aws-iot-device-sdk-js',
  jobId: '1',
  status: 'IN_PROGRESS',
  statusDetails: { step: 'downloading', fileName: 'new-firmware.rpm' }
}
updateJobStatus: {
  thingName: 'aws-iot-device-sdk-js',
  jobId: '1',
  status: 'IN_PROGRESS',
  statusDetails: { operation: 'install', step: 'restarting package' }
}
updateJobStatus: {
  thingName: 'aws-iot-device-sdk-js',
  jobId: '1',
  status: 'SUCCEEDED',
  statusDetails: { operation: 'install', state: 'package installed and started' }
}
```

2. 查看 IoT Job 状态

```shell
$ aws iot describe-job --job-id 1
{
    "documentSource": "https://bucket-name.s3.cn-north-1.amazonaws.com.cn/jobs-document.json", 
    "job": {
        "status": "COMPLETED", 
        "jobArn": "arn:aws-cn:iot:cn-north-1:408221054609:job/1", 
        "completedAt": 1566886074.239, 
        "jobProcessDetails": {
            "numberOfQueuedThings": 0, 
            "numberOfInProgressThings": 0, 
            "numberOfSucceededThings": 1, 
            "numberOfCanceledThings": 0, 
            "numberOfFailedThings": 0, 
            "numberOfRemovedThings": 0, 
            "numberOfRejectedThings": 0
        }, 
        "presignedUrlConfig": {
            "expiresInSec": 300, 
            "roleArn": "arn:aws-cn:iam::408221054609:role/iot-access-s3"
        }, 
        "jobId": "1", 
        "lastUpdatedAt": 1566886074.239, 
        "targetSelection": "SNAPSHOT", 
        "jobExecutionsRolloutConfig": {}, 
        "targets": [
            "arn:aws-cn:iot:cn-north-1:408221054609:thing/aws-iot-device-sdk-js"
        ], 
        "createdAt": 1566886070.087
    }
}
```

3. 查看固件版本号

```shell
# 程序版本已由 telnet-0.17-48.el6.x86_64 升级到 telnet-0.17-64.el7.x86_64
$ rpm -qa |grep telnet
telnet-0.17-64.el7.x86_64
```

到此为止，通过以上几步简单的动手环节，您已成功的完成了 OTA 升级。