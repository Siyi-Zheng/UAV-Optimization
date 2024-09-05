import os
import subprocess
import re
import time
from skopt import gp_minimize
from skopt.space import Real

def run_agent_and_get_energy(waypoint1, waypoint2, wait_time=10, max_retries=6):
    try:
        home_dir = os.path.expanduser('~')
        os.chdir(home_dir)

        build_agent_dir = os.path.join(home_dir, 'ub-anc', 'build-agent')
        os.chdir(build_agent_dir)

        command = f"./build-agent.sh {waypoint1} {waypoint2}"
        subprocess.run(command, shell=True, check=True)

        log_file = os.path.join(home_dir, 'mission_log.txt')
        for _ in range(max_retries):
            if os.path.exists(log_file):
                break
            else:
                time.sleep(wait_time)

        if not os.path.exists(log_file):
            return float('inf')

        with open(log_file, 'r') as file:
            log_content = file.read()

        energy_pattern = r"Total energy consumption:\s*(\d+\.?\d*)"
        match = re.search(energy_pattern, log_content)

        if match:
            total_energy = float(match.group(1))
            return total_energy
        else:
            return float('inf')

    except subprocess.CalledProcessError as e:
        return float('inf')
    except Exception as e:
        return float('inf')

def objective(waypoints):
    waypoint1, waypoint2 = waypoints
    energy = run_agent_and_get_energy(waypoint1, waypoint2)
    return energy

search_space = [Real(0, 1, name='waypoint1'), Real(0, 1, name='waypoint2')]

result = gp_minimize(objective, search_space, n_calls=30, random_state=42)

best_waypoints = result.x
best_energy = result.fun

print(f"Best waypoint positions: {best_waypoints}")
print(f"Minimum energy consumption: {best_energy} kWh")
