#!/usr/bin/env python3
import subprocess
import re
import argparse
import math
import concurrent.futures
import sys

try:
    import cma
except ImportError:
    print("The 'cma' package is not installed. Please install it using 'pip install cma'.")
    sys.exit(1)

def haversine(lat1, lon1, lat2, lon2):
    R = 6378.137 # Equatorial Earth radius in km
    dLat = math.radians(lat2 - lat1)
    dLon = math.radians(lon2 - lon1)
    a = math.sin(dLat/2) * math.sin(dLat/2) + \
        math.cos(math.radians(lat1)) * math.cos(math.radians(lat2)) * \
        math.sin(dLon/2) * math.sin(dLon/2)
    c = 2 * math.atan2(math.sqrt(a), math.sqrt(1-a))
    return R * c

def initial_bearing(lat1, lon1, lat2, lon2):
    """Calculates initial bearing to give the optimizer a good starting point."""
    lat1 = math.radians(lat1)
    lat2 = math.radians(lat2)
    diffLong = math.radians(lon2 - lon1)
    x = math.sin(diffLong) * math.cos(lat2)
    y = math.cos(lat1) * math.sin(lat2) - (math.sin(lat1) * math.cos(lat2) * math.cos(diffLong))
    initial_bearing = math.atan2(x, y)
    return (math.degrees(initial_bearing) + 360) % 360

def run_sim(params, mass, fuel_fraction, area, cd, twr, start_lat, start_lon, target_lat, target_lon, sim_path, other_args):
    pitch, azimuth, isp = params
    
    cmd = [
        sim_path,
        '--lat', str(start_lat),
        '--lon', str(start_lon),
        '--pitch', str(pitch),
        '--azimuth', str(azimuth),
        '--mass', str(mass),
        '--twr', str(twr),
        '--isp', str(isp),
        '--fuel_fraction', str(fuel_fraction),
        '--area', str(area),
        '--cd', str(cd)
    ]
    cmd.extend(other_args)

    try:
        # Run the compiled C simulator
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        # Parse the output
        match = re.search(r"Impact coordinates \(lat, long\)\s*:\s*([-.\d]+),\s*([-.\d]+)", result.stdout)
        if match:
            impact_lat = float(match.group(1))
            impact_lon = float(match.group(2))
            
            # Fitness is the distance between actual impact and desired target
            return haversine(target_lat, target_lon, impact_lat, impact_lon)
        else:
            return 1e6 # High penalty if it didn't impact (e.g., entered orbit or crashed)
    except Exception:
        return 1e6 # High penalty for timeouts or execution errors

def main():
    parser = argparse.ArgumentParser(description="Find optimal launch parameters (pitch, azimuth, isp) to hit a target for a given mass, TWR, and fuel fraction.")
    parser.add_argument('--sim_path', type=str, default='./ballistic_sim', help='Path to simulator executable')
    parser.add_argument('--start_lat', type=float, required=True, help='Launch latitude')
    parser.add_argument('--start_lon', type=float, required=True, help='Launch longitude')
    parser.add_argument('--target_lat', type=float, required=True, help='Target latitude')
    parser.add_argument('--target_lon', type=float, required=True, help='Target longitude')
    parser.add_argument('--mass', type=float, required=True, help='Fixed initial mass of the rocket in kg')
    parser.add_argument('--fuel_fraction', type=float, required=True, help='Fixed mass fraction of the propellant (0.0 to 1.0)')
    parser.add_argument('--area', type=float, required=True, help='Fixed reference area for drag calculations in m^2')
    parser.add_argument('--cd', type=float, required=True, help='Fixed drag coefficient')
    parser.add_argument('--twr', type=float, required=True, help='Fixed initial Thrust-to-Weight Ratio')
    parser.add_argument('--workers', type=int, default=8, help='Number of parallel simulator instances')
    parser.add_argument('--tolerance', type=float, default=1.0, help='Target tolerance in km to stop optimization')
    
    args, unknown_args = parser.parse_known_args()

    # Generate a solid initial guess
    init_azimuth = initial_bearing(args.start_lat, args.start_lon, args.target_lat, args.target_lon)
    init_pitch = 45.0
    init_isp = 300.0
    
    x0 = [init_pitch, init_azimuth, init_isp]
    sigma0 = 10.0 # Initial standard deviation step size
    
    # We optimize [Pitch, Azimuth, ISP]
    options = {
        'bounds': [[0.0, 0.0, 100.0], [90.0, 360.0, 1000.0]],
        'CMA_stds': [10.0, 15.0, 5.0], # Scale the standard deviations for each dimension
        'popsize': max(8, args.workers * 2), # Population size
        'verb_disp': 1
    }

    print(f"Starting CMA-ES optimization using {args.workers} parallel workers...")
    print(f"Fixed Parameters: Mass={args.mass} kg, Fuel Fraction={args.fuel_fraction}, Area={args.area} m^2, CD={args.cd}, TWR={args.twr}")
    print(f"Initial Guess: Pitch={init_pitch}°, Azimuth={init_azimuth:.2f}°, ISP={init_isp} s")
    es = cma.CMAEvolutionStrategy(x0, sigma0, options)

    with concurrent.futures.ThreadPoolExecutor(max_workers=args.workers) as executor:
        generation = 0
        while not es.stop():
            solutions = es.ask()
            
            # Submit simulations in parallel
            futures = [
                executor.submit(run_sim, sol, args.mass, args.fuel_fraction, args.area, args.cd, args.twr, args.start_lat, args.start_lon, args.target_lat, args.target_lon, args.sim_path, unknown_args)
                for sol in solutions
            ]
            
            # Gather fitness values
            fitness = [f.result() for f in futures]
            
            es.tell(solutions, fitness)
            es.disp()
            generation += 1
            
            best_idx = fitness.index(min(fitness))
            print(f"Gen {generation:03d} | Min Dist: {fitness[best_idx]:.6f} km | Pitch: {solutions[best_idx][0]:.3f}°, Az: {solutions[best_idx][1]:.3f}°, ISP: {solutions[best_idx][2]:.3f}s")

            # Stop early if we hit the target within tolerance
            if min(fitness) <= args.tolerance:
                print(f"Target reached within {args.tolerance} km tolerance!")
                break

    best_sol = es.result.xbest
    best_fit = es.result.fbest
    print("\n=== OPTIMIZATION FINISHED ===")
    print(f"Target miss distance : {best_fit:.3f} km")
    print(f"Optimal Pitch        : {best_sol[0]:.3f} deg")
    print(f"Optimal Azimuth      : {best_sol[1]:.3f} deg")
    print(f"Optimal ISP          : {best_sol[2]:.2f} s")
    print(f"\nFixed Parameters:")
    print(f"Mass                 : {args.mass:.2f} kg")
    print(f"Fuel Fraction        : {args.fuel_fraction:.4f}")
    print(f"Area                 : {args.area:.2f} m^2")
    print(f"Drag Coefficient (CD): {args.cd:.3f}")
    print(f"TWR                  : {args.twr:.3f}")
    
    # Output the exact command to run the best result
    cmd_args = f"--lat {args.start_lat:.6f} --lon {args.start_lon:.6f} --pitch {best_sol[0]:.6f} --azimuth {best_sol[1]:.6f} --mass {args.mass} --twr {args.twr} --isp {best_sol[2]:.6f} --fuel_fraction {args.fuel_fraction} --area {args.area} --cd {args.cd}"
    if unknown_args:
        cmd_args += " " + " ".join(unknown_args)
    print(f"\nTo reproduce, run:\n{args.sim_path} {cmd_args}")

if __name__ == '__main__':
    main()