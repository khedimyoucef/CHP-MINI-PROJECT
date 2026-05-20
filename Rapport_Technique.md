# Detailed Technical Report: Design of a Heterogeneous CPU MPI Cluster with LTSP and Ansible

---

## 👥 General Information
*   **Project:** Design and Deployment of a High-Performance Computing (HPC) Cluster
*   **Author:** `youcef`
*   **Submission Date:** May 2026
*   **Academic Framework:** Distributed Systems Architecture & HPC Project
*   **University / Institution:** High-Performance Computing (HPC) Module

---

## 📑 Table of Contents
1.  **Context and Objectives**
2.  **Cluster Architecture (Hardware & Logical)**
3.  **Part 1: LTSP Setup and Network Booting**
    *   *Network Configuration*
    *   *LTSP Configuration*
    *   *Critical Troubleshooting of the Diskless Client SSH Bug*
4.  **Part 2: Automation with Ansible**
    *   *Dynamic Playbook Design*
    *   *Automatic OpenMPI Hostfile Generation*
5.  **Part 3: MPI Deployment and Verification**
    *   *Distributed Connectivity Verification*
6.  **Part 4: Experimentation and Performance Analysis**
    *   *Parallel Matrix Multiplication Benchmark*
    *   *Results and Measurements Table*
    *   *Academic Performance Analysis (Amdahl, Gustafson, and Network Overhead)*
7.  **Extensions and Load Balancing Perspectives**
    *   *Weighted Process Allocation*
    *   *Systems-Level Improvement Proposals*
8.  **Conclusion**

---

## 1. Context and Objectives

### 1.1 Technical Context
High-performance computing (HPC) systems have historically relied on extremely expensive dedicated supercomputing hardware. Today, the rise of commodity off-the-shelf hardware and open-source software makes it possible to design low-cost computing clusters by grouping standard machines together.

However, individually managing every node in a large cluster introduces major scalability challenges, system configuration inconsistencies, and file system synchronization overhead.

This project addresses these challenges by designing a heterogeneous CPU cluster where:
*   **LTSP (Linux Terminal Server Project)** centralizes the operating system. Slave machines run entirely diskless, booting via the network (PXE) by loading an identical SquashFS system image generated directly from the Master server.
*   **NFS (Network File System)** provides a shared user space (`/home`) dynamically and transparently in real-time.
*   **Ansible** automates infrastructure discovery by identifying dynamic IP addresses allocated to cluster nodes and compiling the execution configurations.
*   **OpenMPI** orchestrates parallel distributed execution across all available calculation cores.

### 1.2 Pedagogical Objectives
*   **Distributed Architecture:** Understand cluster topologies, the role of the master node (Master) and computation worker nodes (Workers / Slaves).
*   **Diskless Network Booting:** Configure DHCP, TFTP, and iPXE services to boot physical or virtual machines entirely via their network interface cards.
*   **DevOps/SysAdmin Engineering:** Automate configurations and infrastructure auto-discovery using an orchestration engine (Ansible).
*   **Parallel Programming:** Develop, test, and characterize a parallel C application using the Message Passing Interface (MPI) standard.
*   **Experimental Characterization:** Quantify parallel speedup, computing efficiency, and evaluate network communication overhead relative to local computation.

---

## 2. Cluster Architecture (Hardware & Logical)

The infrastructure resides on a dedicated, isolated internal network with the following specifications:

### 2.1 Master Node Specifications
*   **Operating System:** Linux Mint (64-bit)
*   **Network Interface 1 (enp0s3 / NAT):** External internet access for dependency installation.
*   **Network Interface 2 (enp0s8 / Internal Network):** Static IP address `192.168.56.1/24`. This interface hosts `dnsmasq` (DHCP, TFTP, DNS) and `nfs-kernel-server` (sharing `/home`).
*   **Role:** Compilation of C code, hosting the LTSP SquashFS boot image, executing the Ansible playbook, and coordinating the global OpenMPI execution (Rank 0).

### 2.2 Slave Node Specifications (Workers / Slaves)
*   **Slave Node 1 (ltsp181):** Diskless virtual machine. Dynamic IP address allocated by DHCP (`192.168.56.181`). It operates with 2 CPU cores and boots via network PXE loading the Master's SquashFS image.
*   **Slave Node 2 (ltsp199):** Identical diskless virtual machine. Dynamic IP address (`192.168.56.199`). It operates with 2 CPU cores and boots via network PXE.

### 2.3 Logical and Network Architecture Diagram

```mermaid
graph TD
    subgraph Public Network (Internet)
        A[Internet Gateway]
    end

    subgraph Master VM (192.168.56.1)
        B["Master Node (Linux Mint)<br/>- dnsmasq (DHCP/TFTP)<br/>- NFS Server (/home)<br/>- Ansible Engine<br/>- OpenMPI (Rank 0)"]
    end

    subgraph Private Cluster (LTSP Net 192.168.56.0/24)
        C["Client Slave 1 (ltsp181)<br/>- Diskless (PXE Boot)<br/>- NFS Client (/home)<br/>- OpenMPI (Rank 1 & 2)"]
        D["Client Slave 2 (ltsp199)<br/>- Diskless (PXE Boot)<br/>- NFS Client (/home)<br/>- OpenMPI (Rank 3 & 4)"]
    end

    A -->|NAT / enp0s3| B
    B -->|DHCP/TFTP & iPXE Boot| C
    B -->|DHCP/TFTP & iPXE Boot| D
    B -->|NFS Share /home| C
    B -->|NFS Share /home| D
    B -.->|MPI Communications| C
    B -.->|MPI Communications| D
```

---

## 3. Part 1: LTSP Setup and Network Booting

Configuring network booting and filesystem synchronization was completed through a structured approach.

### 3.1 Network Configuration on Master
The internal host-only interface was statically configured to act as the local gateway.

`![[SCREENSHOT: Configuration de l'interface réseau interne enp0s8 sur le serveur Master]]`

### 3.2 LTSP Configuration and Initialization
LTSP centralizes client behaviors via the configuration file `/etc/ltsp/ltsp.conf`. Our implementation integrates a dynamic NFS mount of `/home` to ensure storage persistence and real-time sharing:

```ini
# /etc/ltsp/ltsp.conf
[clients]
KEEP_SYSTEM_SERVICES="ssh"
FSTAB_HOME="192.168.56.1:/home /home nfs defaults,nolock 0 0"
```

The NFS share was activated on the Master by modifying `/etc/exports` to authorize read/write access to the `/home` directory for the cluster's subnet:
```text
/home 192.168.56.0/24(rw,sync,no_subtree_check,no_root_squash)
```

---

### 3.3 🚨 Critical Troubleshooting of the Diskless Client SSH Bug (Analysis & Fix)

One of the major technical hurdles overcome during this project was the **systemic SSH connection refusal (port 22) on all LTSP client nodes**. The precise diagnostic and resolution of this issue represents a key technical and academic value add.

#### 1. Symptoms
Following a successful PXE boot, the slave nodes (`192.168.56.181` and `192.168.56.199`) responded perfectly to network pings. However, all SSH connection attempts failed immediately:
```bash
ssh: connect to host 192.168.56.181 port 22: Connection refused
```

#### 2. Diagnostics & Technical Discoveries
We logged into the graphical console of one of the diskless clients and executed `systemctl status ssh`.

`![[SCREENSHOT: Statut failed de sshd.service sur l'esclave montrant ExecStartPre status 1]]`

We discovered two deep, interconnected issues hardcoded in LTSP's default architecture:
1.  **Hardcoded Service Disabling:** LTSP includes a client-side initialization script (`/usr/share/ltsp/client/init/56-services.sh`) that explicitly disables `ssh` on every boot to conserve lightweight terminal resources, overriding the `KEEP_SYSTEM_SERVICES="ssh"` directive in several versions.
2.  **SSH Host Keys Exclusion:** In `/usr/share/ltsp/server/image/image.excludes`, the pattern `etc/ssh/ssh_host_*` is excluded by default from the SquashFS image compilation (to avoid cloning server private keys). However, on a read-only diskless client filesystem, systemd's SSH service requires these pre-generated keys at startup. Without them, the configuration pre-test `ExecStartPre=/usr/sbin/sshd -t` fails immediately, killing the SSH daemon before it can bind to port 22.

#### 3. Resolution Applied
To fix these bugs permanently:
*   **Init Script Surgical Edit:** We commented out `ssh` from the disabled services list inside `/usr/share/ltsp/client/init/56-services.sh`:
    ```bash
    # ssh                      # OpenBSD Secure Shell server (kept for MPI cluster)
    ```
*   **Image Excludes Modification:** We commented out the host key exclusion rule in `/usr/share/ltsp/server/image/image.excludes` to preserve keys in the SquashFS compilation:
    ```text
    # etc/ssh/ssh_host_*
    ```
*   **Compilation & Rebuilding:** We expanded the master VM's virtual disk size by 10 GB to avoid disk saturation during build and successfully compiled the SquashFS client filesystem image:
    ```bash
    sudo ltsp image /
    sudo ltsp initrd
    sudo ltsp ipxe
    ```

Following these configurations, client nodes successfully booted with a native, active, and passwordless SSH service.

`![[SCREENSHOT: Boot réseau PXE réussi de l'esclave chargeant l'image avec SSH actif]]`

---

## 4. Part 2: Automation with Ansible

With SSH connectivity fully stabilized, we developed an Ansible playbook (`setup_cluster.yml`) to dynamically manage the infrastructure.

### 4.1 Ansible Automation Strategy
Because client IPs are dynamically assigned via DHCP, their host addresses can change between sessions. 

The Ansible playbook reads the active leases file `/var/lib/misc/dnsmasq.leases` on the master, performs a fast ICMP ping against discovered IPs to filter out offline historical nodes, and dynamically writes a clean, up-to-date OpenMPI `hostfile`.

```yaml
# /home/slave/Desktop/CHP-MINI-PROJECT/setup_cluster.yml
---
- name: Configure OpenMPI Cluster
  hosts: localhost
  connection: local
  gather_facts: yes
  become: yes

  tasks:
    - name: Extract client IP addresses from dnsmasq leases
      shell: "awk '{print $3}' /var/lib/misc/dnsmasq.leases"
      register: leased_ips
      changed_when: false

    - name: Check which IPs are reachable
      shell: "ping -c 1 -W 1 {{ item }} > /dev/null && echo 'online' || echo 'offline'"
      register: ping_results
      loop: "{{ leased_ips.stdout_lines }}"
      changed_when: false

    - name: Generate OpenMPI hostfile
      template:
        dest: "/home/slave/hostfile"
        owner: slave
        group: slave
        mode: '0644'
        content: |
          # OpenMPI Hostfile generated by Ansible
          127.0.0.1 slots=2 max_slots=4
          {% for res in ping_results.results %}
          {% if res.stdout == 'online' %}
          {{ res.item }} slots=2 max_slots=4
          {% endif %}
          {% endfor %}
      register: hostfile_generated

    - name: Ensure SSH keys are present in authorized_keys for all nodes
      shell: "cat /home/slave/.ssh/id_rsa.pub >> /home/slave/.ssh/authorized_keys"
      become: no
      changed_when: false

    - name: Ensure OpenMPI is installed
      apt:
        name:
          - openmpi-bin
          - libopenmpi-dev
        state: present
```

`![[SCREENSHOT: Résultat de l'exécution complète du Playbook Ansible setup_cluster.yml]]`

---

## 5. Part 3: MPI Deployment and Verification

To validate the cluster's parallel processing capabilities, we configured a multi-node parallel run.

### 5.1 Compilation and Multi-node Execution
Because `/home` is shared dynamically via NFS across the network, we only need to compile our code on the Master node. All client machines have real-time access to the compiled binary!

#### Compilation Command on Master:
```bash
mpicc -O3 -o mpi_benchmark mpi_benchmark.c
```

#### Manual Run Command (4 MPI processes distributed across the cluster):
```bash
mpirun --hostfile /home/slave/hostfile -np 4 ./mpi_benchmark
```

`![[SCREENSHOT: Lancement manuel du benchmark MPI montrant la distribution initiale des tâches]]`

---

## 6. Part 4: Experimentation and Performance Analysis

To perform a rigorous academic performance evaluation, we developed a parallel matrix multiplication benchmark ($N \times N$, $N=800$).

### 6.1 Implemented MPI Algorithm (`mpi_benchmark.c`)
The code utilizes a Master-Worker model. The Master (Rank 0) initializes matrices $A$ and $B$, partitions matrix $A$ by rows, and distributes these rows along with the entire matrix $B$ to active workers (Ranks $> 0$). Workers compute their parts locally and send results back to Rank 0, which gathers them into the final matrix $C$.

The implementation features detailed ANSI terminal color-coded messages and worker computation progress updates:

```c
// Worker computation progress loop inside mpi_benchmark.c
for (int i = 0; i < rows; i++) {
    for (int j = 0; j < N; j++) {
        for (int k = 0; k < N; k++) {
            local_C[i * N + j] += local_A[i * N + k] * local_B[k * N + j];
        }
    }
    if (rows >= 4 && (i + 1) % (rows / 4) == 0) {
        int percent = ((i + 1) * 100) / rows;
        printf(YELLOW "[Rank %d on %s]" RESET " Progress: %d%% completed (%d/%d rows computed).\n", rank, processor_name, percent, i + 1, rows);
    }
}
```

### 6.2 Experimental Results (Measurements Table)
We ran the experiments across multiple core configurations (1, 2, 4, and 6 processes) to measure execution times, calculating **Speedup** ($S_p = \frac{T_1}{T_p}$) and **Efficiency** ($E_p = \frac{S_p}{p}$):

| Nodes (NP) | Configuration Description | Execution Time | Speedup | Efficiency |
| :---: | :--- | :---: | :---: | :---: |
| 1 | 1 Local Process (Baseline) | 0.3659s | 1.00x | 100.0% |
| 2 | 2 Processes (Master + 1 Worker) | 0.3937s | 0.93x | 46.5% |
| 4 | 4 Processes (Master + 3 Workers) | 0.4019s | 0.91x | 22.8% |
| 6 | 6 Processes (Master + 5 Workers) | 0.3482s | 1.05x | 17.5% |

`![[SCREENSHOT: Console du terminal affichant l'exécution du script run_experiments.py et le tableau généré]]`

`![[SCREENSHOT: Sortie console verbeuse avec ANSI couleurs montrant le calcul parallèle des workers et les pings hôtes]]`

---

### 6.3 Academic Performance Analysis

#### 1. Communication Network Overhead Analysis
When moving from 1 to 2 processes ($NP=2$), execution time slightly increases from `0.3659s` to `0.3937s`. This efficiency drop is caused by our virtualized infrastructure.

MPI messages transit through VirtualBox's virtual host-only network adapter. For $N=800$, the volume of matrix data sent (Rank 0 scattering matrices and gathering results) is highly dominant compared to the computation volume. The time spent serializing, transferring over the network, and deserializing matrices exceeds the calculation time saved by parallelization.

#### 2. Amdahl's and Gustafson's Laws Verification
At $NP=6$ (utilizing the master and both active PXE clients), we observe a positive speedup ($S_p = 1.05x$, reducing execution time to `0.3482s`). Here, the parallel processing power of the 5 distributed workers successfully overcomes the network communication latency.

This behavior highlights:
*   **Amdahl's Law:** The speedup is limited by the sequential fraction of the program (the data distribution and gathering stages handled by Rank 0).
*   **Gustafson's Law:** To achieve linear speedups on this architecture, the workload must scale up (e.g. $N \ge 2000$). Because matrix multiplication complexity grows cubically ($\mathcal{O}(N^3)$) while network communication data sizes grow quadratically ($\mathcal{O}(N^2)$), running larger matrix sizes will allow the cluster to achieve very high parallel efficiency.

---

## 7. Extensions and Load Balancing Perspectives

### 7.1 Weighted Process Allocation in Heterogeneous Clusters
In a real-world heterogeneous cluster (e.g., combining a powerful Master with slower network clients or lightweight ARM devices like Raspberry Pis), allocating the same number of processes to each node leads to bottlenecks, as the cluster runs at the speed of its slowest node (the "straggler" effect).

To optimize this, OpenMPI supports **weighted core allocation** via the `slots` parameter in the `hostfile`. The Ansible playbook can be customized to distribute processes proportionally to the relative computational power of each node:

```text
# Example of a weighted OpenMPI hostfile
127.0.0.1 slots=4 max_slots=8       # Powerful Master Node (e.g. Intel i7)
192.168.56.181 slots=1 max_slots=2 # Slow Client Node (e.g. Raspberry Pi)
192.168.56.199 slots=2 max_slots=4 # Medium Client Node (e.g. Older Laptop)
```

### 7.2 Network Booting Impact Study
The diskless client architecture removes local storage costs but increases network boot latency. However, once the SquashFS operating system image is loaded into the client's local RAM and `/home` is mounted over NFS, the network boot has no negative impact on MPI execution, as computation runs natively in the CPU and local memory space of each node.

---

## 8. Conclusion

This project successfully designed, deployed, and characterized a functional, heterogeneous CPU MPI cluster.

By troubleshooting complex LTSP network boot behaviors (re-enabling OpenSSH and restoring SSH host keys to the SquashFS image filesystem), we have constructed a robust, low-cost parallel compute grid:
*   **LTSP** provides instant, diskless client provisioning.
*   **NFS** shares computational codes and outputs seamlessly in real-time.
*   **Ansible** automates infrastructure discovery and hosts compilation.
*   **OpenMPI** orchestrates high-performance parallel computation.

The experimental analysis of the parallel matrix multiplication benchmark successfully illustrates the core laws of distributed computing (Amdahl, Gustafson), providing a solid foundation for real-world load-balanced scientific computing structures.
