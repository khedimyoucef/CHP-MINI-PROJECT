Conception d’un Cluster MPI Hétérogène CPU avec LTSP et Ansible
1. Contexte
Les systèmes de calcul haute performance (HPC) reposent sur des architectures distribuées permettant l’exécution parallèle de programmes intensifs.
Dans ce projet, vous allez concevoir et déployer un cluster CPU hétérogène à faible coût, en utilisant :
- LTSP (Linux Terminal Server Project)
- Ansible (outil d’automatisation)
- OpenMPI (programmation parallèle distribuée)
 
2. Objectifs pédagogiques
- Comprendre l’architecture d’un cluster distribué
- Déployer un système Linux via boot réseau
- Automatiser la configuration d’un ensemble de machines
- Exécuter et analyser un programme MPI
- Évaluer l’impact de l’hétérogénéité sur les performances
 
3. Architecture attendue
Infrastructure matérielle :
- 1 machine serveur (Master)
- Au moins 2 machines clientes
- Machines hétérogènes possibles (PC x86, Raspberry Pi ARM, etc.)
 
Architecture logique :
- Serveur : image LTSP, automatisation Ansible, OpenMPI
- Clients : boot PXE, configuration automatique, participation MPI
 
4. Travail demandé
Partie 1 — Mise en place
- Installation serveur Linux
- Configuration LTSP
- Boot réseau des clients
- Configuration SSH
 
Partie 2 — Automatisation
- Création playbook Ansible
- Installation OpenMPI
- Génération hostfile automatique
 
Partie 3 — Déploiement MPI
- Tests multi-nœuds
 
Partie 4 — Expérimentation
- HPCG , HPL, ….
- Mesure : temps, speedup, efficacité
 
5. Livrables
- Rapport technique (10–15 pages)
- Code source MPI
- Playbooks Ansible
- Diagramme d’architecture
 
6. Critères d’évaluation
Fonctionnement cluster : 25%
Automatisation : 20%
Exécution MPI : 20%
Analyse expérimentale : 25%
Qualité rapport : 10%
 
7. Extensions possibles
- Allocation pondérée des processus MPI
- Étude impact boot réseau
- Proposition d’amélioration d’équilibrage

Bon courage.