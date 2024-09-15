Sliding-Window
==============

Sliding Window: implements the sender and the receiver that communicate using the GBN protocol with UDP
One more thing I sort of miss interpret the sliding part, it really does it in chunks.

There are 2 ways to run "Sliding-Window"
1. Use containers (more repoducable, and easier to setup and run)
2. Use "mininet" application (this was the original assignment)

I'll describe how to use as containers method below. Its a more hands on, sorta aproach.
Build the containers,
- `docker build -t sw_cli:1 -f ./Client/Dockerfile .`
- `docker build -t sw_ser:1 -f ./Server/Dockerfile .`

Check the network, `docker network ls`, there should be a default one called "bridge". If not then create one `docker network create -d bridge some_bridge`.
NETWORK ID     NAME                                                           DRIVER    SCOPE
xxxxxxxxxxxx   bridge                                                         bridge    local

Run the container,
- `docker run --rm -it --net=bridge --name sw_cli sw_cli:1 /bin/bash`
- `docker run --rm -it --net=bridge --name sw_ser sw_ser:1 /bin/bash`

Check the server ip, `docker network inspect bridge`
```
[
    {
        "Name": "bridge",
        ...
        "Containers": {
            "9ec252714fec24afbd39b7a25e40f25763261df297e281ebf7ca290069e83c0c": {
                "Name": "sw_cli",
                ...
                "IPv4Address": "172.17.0.3/16",
                ...
            },
            "e26c8dfeb979c654534f656a32db10ad87df60ff41f77de4b5002367a3fd7828": {
                "Name": "sw_ser",
                ...
                "IPv4Address": "172.17.0.2/16",
                ...
            }
        },
    ...
    }
]
```

Start the client, `./build/client 172.17.0.2 3000 ./build/client`
Start the server, `./build/server 3000 client`

```
***** Sending *****
Sending File: ./build/client
File size 21.18 Kb
Throughput: 484.53 bits/sec   
***** Closing Connection *****
```

```
***** Receiving *****
Writting File: client
Transfer Size: 21.18 Kb       
Completed
***** Closing Connection *****
```