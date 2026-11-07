"""Run the OMP reference and the MLX port on the same k-Wave inputs and compare all output datasets.

Usage: python compare.py <omp_binary> <mlx_binary> <inputs_dir> <outputs_dir> [case_filter]
"""
import os
import subprocess
import sys
import time

import h5py
import numpy as np

FLAGS = ["--p_raw", "--p_rms", "--p_max", "--p_min", "--p_max_all", "--p_min_all", "--p_final",
         "--u_raw", "--u_non_staggered_raw", "--u_rms", "--u_max", "--u_min", "--u_max_all", "--u_min_all",
         "--u_final", "--copy_sensor_mask"]


def run(binary, inp, out, extra_env=None):
    env = dict(os.environ)
    if extra_env:
        env.update(extra_env)
    t0 = time.time()
    res = subprocess.run([binary, "-i", inp, "-o", out] + FLAGS, capture_output=True, text=True, env=env)
    elapsed = time.time() - t0
    if res.returncode != 0:
        print(res.stdout[-3000:])
        print(res.stderr[-3000:])
        raise RuntimeError(f"{binary} failed on {inp}")
    return elapsed


def collect(f):
    data = {}

    def visit(name, obj):
        if isinstance(obj, h5py.Dataset):
            data[name] = np.asarray(obj[()])
    f.visititems(visit)
    return data


def compare(ref_file, test_file):
    worst = 0.0
    rows = []
    with h5py.File(ref_file, "r") as fr, h5py.File(test_file, "r") as ft:
        ref, test = collect(fr), collect(ft)
        missing = sorted(set(ref) - set(test))
        extra = sorted(set(test) - set(ref))
        for name in sorted(set(ref) & set(test)):
            a, b = ref[name], test[name]
            if a.shape != b.shape:
                rows.append((name, "SHAPE MISMATCH", f"{a.shape} vs {b.shape}"))
                worst = np.inf
                continue
            if a.dtype.kind in "iu":
                ok = np.array_equal(a, b)
                rows.append((name, "exact" if ok else "INDEX MISMATCH", ""))
                if not ok:
                    worst = np.inf
                continue
            a = a.astype(np.float64)
            b = b.astype(np.float64)
            scale = np.max(np.abs(a))
            if scale == 0:
                err = np.max(np.abs(b))
            else:
                err = np.max(np.abs(a - b)) / scale
            l2 = np.linalg.norm(a - b) / max(np.linalg.norm(a), 1e-30)
            worst = max(worst, err)
            rows.append((name, f"max rel {err:.2e}", f"L2 rel {l2:.2e}  (|ref|max {scale:.3e}, shape {a.shape})"))
    return worst, rows, missing, extra


def main():
    omp, mlx_bin, inputs, outputs = sys.argv[1:5]
    flt = sys.argv[5] if len(sys.argv) > 5 else ""
    os.makedirs(outputs, exist_ok=True)
    summary = []
    for case in sorted(os.listdir(inputs)):
        if not case.endswith(".h5") or flt not in case:
            continue
        name = case[:-3]
        inp = os.path.join(inputs, case)
        out_ref = os.path.join(outputs, name + "_omp.h5")
        out_mlx = os.path.join(outputs, name + "_mlx.h5")
        t_ref = run(omp, inp, out_ref)
        t_mlx = run(mlx_bin, inp, out_mlx)
        worst, rows, missing, extra = compare(out_ref, out_mlx)
        print(f"\n=== {name}   (OMP {t_ref:.2f}s, MLX {t_mlx:.2f}s)")
        for r in rows:
            print(f"  {r[0]:28s} {r[1]:18s} {r[2]}")
        if missing:
            print("  MISSING in MLX output:", missing)
        if extra:
            print("  EXTRA in MLX output:", extra)
        summary.append((name, worst, t_ref, t_mlx, missing))
    print("\n=== Summary (worst max-relative error over all datasets)")
    for name, worst, t_ref, t_mlx, missing in summary:
        print(f"  {name:40s} {worst:.2e}   OMP {t_ref:6.2f}s  MLX {t_mlx:6.2f}s {'MISSING DATA' if missing else ''}")


if __name__ == "__main__":
    main()
