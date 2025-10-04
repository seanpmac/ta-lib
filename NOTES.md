# GPU Acceleration Notes

## Mac (Apple Silicon)
- Current Metal backend now checks `supports64BitFloat`; if the device lacks FP64 hardware (all M-series GPUs), the double-precision kernels are skipped and Metal falls back to the float32 path.
- `ta_perf` reports `Acceleration: disabled/not available` on macOS in parity mode, because the project keeps double-precision parity and therefore remains on the CPU when float-only acceleration would diverge.
- Enabling Metal validation (`MTL_DEBUG_LAYER=1`) crashes whenever FP64 kernels are requested; this is expected on Apple hardware.
- To keep parity on Apple hardware, leave the build CPU-only or accept float32 tolerances explicitly.

## PC Roadmap (NVIDIA 2080 Ti Super)
- Install CUDA Toolkit and extend the build to compile/link a CUDA backend.
- Implement `ta_accel_cuda` with FP64 kernels (start with SMA via prefix scan/rolling sum).
- Integrate the CUDA entry points into `ta_accel_execute_plan` and `ta_perf`.
- Convert public `decimal` inputs to `double` before GPU execution; convert results back to `decimal` on the way out.
- Reuse the existing regression harness to verify CPU vs CUDA parity.
- Update performance report tooling to expose CUDA timings when available.

## Libraries to Refactor Later
- Skender.Stock.Indicators (C#) ++ Lean indicators use `decimal` by default. The same CPU-to-GPU conversion strategy (decimal ↔ double) will be needed if we accelerate them.
- MLX/Metal on Mac can still be used for float32 workloads, but expect lower precision.

## Next Session Checklist (PC)
1. `cmake --build build-cuda` (new target once CUDA files are added).
2. Implement CUDA SMA kernel and wrapper (`ta_accel_cuda.cu`).
3. Wire CUDA backend into the acceleration plan and benchmarking code.
4. Run `ta_regtest` and CUDA-specific tests to ensure parity.
5. Regenerate `dist/accel_report-cuda.md` once CUDA timings are available.
