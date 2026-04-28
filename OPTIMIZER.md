# Parameter Optimization using `find_params_for_coords.py`

A Python script `find_params_for_coords.py` is included to automatically find the optimal launch parameters (`pitch`, `azimuth`, `isp`) required to hit a specific target latitude and longitude, given a set of fixed rocket characteristics.

It uses the Covariance Matrix Adaptation Evolution Strategy (CMA-ES) algorithm to efficiently search the parameter space. The script runs multiple instances of `ballistic_sim` in parallel to evaluate the fitness of different parameter sets, making the optimization process significantly faster.

## Dependencies

- Python 3
- `cma` Python library

Install the `cma` library using pip:
```bash
pip3 install cma
```

## Usage

The script requires the launch coordinates, target coordinates, and fixed parameters for the rocket (mass, TWR, etc.).

```bash
./find_params_for_coords.py [options]
```

### Optimizer Options:

- `--start_lat <val>`: Launch latitude (required).
- `--start_lon <val>`: Launch longitude (required).
- `--target_lat <val>`: Target latitude (required).
- `--target_lon <val>`: Target longitude (required).
- `--mass <val>`: Fixed initial mass of the rocket in kg (required).
- `--fuel_fraction <val>`: Fixed mass fraction of the propellant (required).
- `--area <val>`: Fixed reference area for drag in m^2 (required).
- `--cd <val>`: Fixed drag coefficient (required).
- `--twr <val>`: Fixed initial Thrust-to-Weight Ratio (required).
- `--workers <val>`: Number of parallel simulator instances to run (default: 8).
- `--tolerance <val>`: Target miss distance in km to stop optimization (default: 1.0).
- `--sim_path <path>`: Path to the `ballistic_sim` executable (default: `./ballistic_sim`).

Any other arguments recognized by `ballistic_sim` (e.g., `--f107A`, `--ap`) can also be passed, and they will be forwarded to the simulator during optimization.

### Example

Find the `pitch`, `azimuth`, and `isp` to launch a 2000 kg rocket from Cape Canaveral to hit a target near London, UK.

```bash
./find_params_for_coords.py \
    --start_lat 28.49156 --start_lon -80.54689 \
    --target_lat 39.710795421069044 --target_lon -31.11174146552424 \
    --mass 2000 --fuel_fraction 0.92 --twr 1.3 \
    --area 0.8 --cd 0.25 --workers 12
```

Example output:
```
Starting CMA-ES optimization using 10 parallel workers...
Fixed Parameters: Mass=50000.0 kg, Fuel Fraction=0.9, Area=1.0 m^2, CD=0.01, TWR=1.5
Initial Guess: Pitch=45.0°, Azimuth=30.18°, ISP=300.0 s
Iterat #Fevals   function value  axis ratio  sigma  min&max std  t[m:s]
    1     20 5.519775591228148e+03 1.0e+00 1.23e+01  3e+01  1e+02 0:00.2
Gen 001 | Min Dist: 5519.775591 km | Pitch: 89.785°, Az: 54.233°, ISP: 279.329s
    2     40 8.900924636310814e+03 1.9e+00 1.30e+01  3e+01  1e+02 0:00.3
Gen 002 | Min Dist: 8900.924636 km | Pitch: 89.348°, Az: 92.050°, ISP: 225.877s
    3     60 8.978645036378224e+03 2.6e+00 1.33e+01  3e+01  8e+01 0:00.4
Gen 003 | Min Dist: 8978.645036 km | Pitch: 87.697°, Az: 4.667°, ISP: 222.552s

...

Gen 065 | Min Dist: 5.086819 km | Pitch: 89.989°, Az: 84.728°, ISP: 292.646s
   66   1584 2.618599968491988e+00 7.1e+02 3.34e+00  1e-03  4e-01 0:45.4
Gen 066 | Min Dist: 2.618600 km | Pitch: 89.989°, Az: 84.560°, ISP: 292.044s
   67   1608 8.178650385901493e-01 6.7e+02 2.86e+00  8e-04  3e-01 0:46.7
Gen 067 | Min Dist: 0.817865 km | Pitch: 89.989°, Az: 84.500°, ISP: 291.846s
Target reached within 1.0 km tolerance!

=== OPTIMIZATION FINISHED ===
Target miss distance : 0.818 km
Optimal Pitch        : 89.989 deg
Optimal Azimuth      : 84.500 deg
Optimal ISP          : 291.85 s

Fixed Parameters:
Mass                 : 2000.00 kg
Fuel Fraction        : 0.9200
Area                 : 0.80 m^2
Drag Coefficient (CD): 0.250
TWR                  : 1.300

To reproduce, run:
./ballistic_sim --lat 28.491560 --lon -80.546890 --pitch 89.989352 --azimuth 84.499551 --mass 2000.0 --twr 1.3 --isp 291.846344 --fuel_fraction 0.92 --area 0.8 --cd 0.25
```

The script will output the progress of the optimization and, upon completion, print the best parameters found and the final miss distance.