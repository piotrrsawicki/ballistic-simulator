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
    --area 0.8 --cd 0.25 --workers 12 --tolerance 0.001
```

Example output:
```
Starting CMA-ES optimization using 10 parallel workers...
Fixed Parameters: Mass=50000.0 kg, Fuel Fraction=0.9, Area=1.0 m^2, CD=0.01, TWR=1.5
Initial Guess: Pitch=45.0°, Azimuth=30.18°, ISP=300.0 s
Iterat #Fevals   function value  axis ratio  sigma  min&max std  t[m:s]
    1     24 4.660331571756759e+03 1.0e+00 1.20e+01  3e+01  1e+02 0:00.1
Gen 001 | Min Dist: 4660.331572 km | Pitch: 89.724°, Az: 12.495°, ISP: 261.289s
    2     48 4.660053962781061e+03 1.7e+00 1.28e+01  3e+01  1e+02 0:00.2
Gen 002 | Min Dist: 4660.053963 km | Pitch: 89.572°, Az: 53.091°, ISP: 261.943s
    3     72 4.660496175449724e+03 1.9e+00 1.54e+01  3e+01  1e+02 0:00.3
Gen 003 | Min Dist: 4660.496175 km | Pitch: 89.633°, Az: 76.146°, ISP: 455.284s
    4     96 4.653138363433909e+03 2.1e+00 1.45e+01  2e+01  9e+01 0:00.4
Gen 004 | Min Dist: 4653.138363 km | Pitch: 89.933°, Az: 56.046°, ISP: 346.665s
    5    120 4.655916267274815e+03 2.4e+00 1.25e+01  1e+01  6e+01 0:00.6
Gen 005 | Min Dist: 4655.916267 km | Pitch: 89.970°, Az: 13.054°, ISP: 359.261s

...

Gen 074 | Min Dist: 0.001877 km | Pitch: 89.989°, Az: 83.653°, ISP: 291.675s
   75   1800 5.268803218802671e-03 1.7e+04 8.85e-02  4e-05  2e-02 1:10.9
Gen 075 | Min Dist: 0.005269 km | Pitch: 89.989°, Az: 83.656°, ISP: 291.664s
   76   1824 5.872140498825716e-03 2.7e+04 8.57e-02  3e-05  1e-02 1:12.2
Gen 076 | Min Dist: 0.005872 km | Pitch: 89.989°, Az: 83.659°, ISP: 291.653s
   77   1848 7.618546337655499e-04 2.8e+04 7.27e-02  3e-05  1e-02 1:13.4
Gen 077 | Min Dist: 0.000762 km | Pitch: 89.989°, Az: 83.652°, ISP: 291.678s
Target reached within 0.001 km tolerance!

=== OPTIMIZATION FINISHED ===
Target miss distance : 0.001 km
Optimal Pitch        : 89.989 deg
Optimal Azimuth      : 83.652 deg
Optimal ISP          : 291.68 s

Fixed Parameters:
Mass                 : 2000.00 kg
Fuel Fraction        : 0.9200
Area                 : 0.80 m^2
Drag Coefficient (CD): 0.250
TWR                  : 1.300

To reproduce, run:
./ballistic_sim --lat 28.491560 --lon -80.546890 --pitch 89.989006 --azimuth 83.652042 --mass 2000.0 --twr 1.3 --isp 291.677615 --fuel_fraction 0.92 --area 0.8 --cd 0.25
```

The script will output the progress of the optimization and, upon completion, print the best parameters found and the final miss distance.