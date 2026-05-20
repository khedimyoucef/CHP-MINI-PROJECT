import subprocess
import time
import sys
import re

def run_command(cmd):
    try:
        result = subprocess.run(cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        return result.stdout, result.stderr, result.returncode
    except Exception as e:
        return "", str(e), -1

def main():
    print("==================================================")
    print("      HPC MPI HETEROGENEOUS CLUSTER BENCHMARK     ")
    print("==================================================")
    
    # 1. Compile the benchmark
    print("[*] Compiling mpi_benchmark.c...")
    stdout, stderr, code = run_command("mpicc -O3 -o mpi_benchmark mpi_benchmark.c")
    if code != 0:
        print(f"[!] Compilation failed:\n{stderr}")
        sys.exit(1)
    print("[+] Compilation successful.")

    # Configurations to test: (number of processes, slots description)
    configs = [
        (1, "1 Local Process (Baseline)"),
        (2, "2 Processes (Master + 1 Worker)"),
        (4, "4 Processes (Master + 3 Workers)"),
        (6, "6 Processes (Master + 5 Workers)")
    ]

    results = []
    baseline_time = None

    for np, desc in configs:
        print(f"\n[*] Running with {np} processes ({desc})...")
        
        # If np == 1, we don't need hostfile, run locally
        if np == 1:
            cmd = "./mpi_benchmark"
        else:
            cmd = f"mpirun --hostfile /home/slave/hostfile -np {np} ./mpi_benchmark"
            
        print(f"    Command: {cmd}")
        stdout, stderr, code = run_command(cmd)
        
        if code != 0:
            print(f"    [!] Failed to execute: {stderr}")
            results.append((np, desc, "FAILED", "-", "-"))
            continue
            
        # Parse time from output (e.g. "Parallel calculation finished in 1.2345 seconds.")
        match = re.search(r"finished in ([\d\.]+) seconds", stdout)
        if match:
            exec_time = float(match.group(1))
            if np == 1:
                baseline_time = exec_time
                speedup = 1.0
                efficiency = 100.0
            else:
                if baseline_time is not None:
                    speedup = baseline_time / exec_time
                    efficiency = (speedup / np) * 100.0
                else:
                    speedup = "-"
                    efficiency = "-"
            
            # Format speedup/efficiency
            speedup_str = f"{speedup:.2f}x" if isinstance(speedup, float) else "-"
            eff_str = f"{efficiency:.1f}%" if isinstance(efficiency, float) else "-"
            
            print(f"    [+] Time: {exec_time:.4f}s | Speedup: {speedup_str} | Efficiency: {eff_str}")
            results.append((np, desc, f"{exec_time:.4f}s", speedup_str, eff_str))
        else:
            print(f"    [!] Could not parse execution time from output:\n{stdout}")
            results.append((np, desc, "PARSE_ERROR", "-", "-"))

    # 3. Output results table
    print("\n" + "="*70)
    print("                        EXPERIMENT RESULTS")
    print("="*70)
    print(f"{'NP':<5} | {'Configuration Description':<32} | {'Time':<10} | {'Speedup':<8} | {'Efficiency':<10}")
    print("-"*70)
    for np, desc, t, sp, eff in results:
        print(f"{np:<5} | {desc:<32} | {t:<10} | {sp:<8} | {eff:<10}")
    print("="*70)

    # 4. Generate Markdown Table for Report
    print("\n[*] Markdown table for Rapport_Technique.md:")
    print("\n| Nœuds (NP) | Description Configuration | Temps d'exécution | Speedup | Efficacité |")
    print("| :---: | :--- | :---: | :---: | :---: |")
    for np, desc, t, sp, eff in results:
        print(f"| {np} | {desc} | {t} | {sp} | {eff} |")
    print()

if __name__ == "__main__":
    main()
