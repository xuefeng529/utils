etcd --name infra0 --initial-advertise-peer-urls http://127.0.0.1:2380 \
  --listen-peer-urls http://127.0.0.1:2380 \
  --listen-client-urls http://127.0.0.1:2377 \
  --advertise-client-urls http://127.0.0.1:2377 \
  --initial-cluster-token etcd-cluster-1 \
  --initial-cluster infra0=http://127.0.0.1:2380,infra1=http://127.0.0.1:2381,infra2=http://127.0.0.1:2382 \
  --initial-cluster-state new
  
  
  etcd --name infra1 --initial-advertise-peer-urls http://127.0.0.1:2381 \
  --listen-peer-urls http://127.0.0.1:2381 \
  --listen-client-urls http://127.0.0.1:2378 \
  --advertise-client-urls http://127.0.0.1:2378 \
  --initial-cluster-token etcd-cluster-1 \
  --initial-cluster infra0=http://127.0.0.1:2380,infra1=http://127.0.0.1:2381,infra2=http://127.0.0.1:2382 \
  --initial-cluster-state new
  
  etcd --name infra2 --initial-advertise-peer-urls http://127.0.0.1:2382 \
  --listen-peer-urls http://127.0.0.1:2382 \
  --listen-client-urls http://127.0.0.1:2379 \
  --advertise-client-urls http://127.0.0.1:2379 \
  --initial-cluster-token etcd-cluster-1 \
  --initial-cluster infra0=http://127.0.0.1:2380,infra1=http://127.0.0.1:2381,infra2=http://127.0.0.1:2382 \
  --initial-cluster-state new