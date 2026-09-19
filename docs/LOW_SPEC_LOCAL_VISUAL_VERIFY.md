# Low-spec local visual verification

This launcher is only for local verification on legacy Windows GPUs.
It does not change the committed cinematic renderer defaults.

Run:

    cd D:\\LifeLens
    powershell -ExecutionPolicy Bypass -File .\\Tools\\run_lifelens_low_spec.ps1

The launcher forces D3D11 and disables Nanite, Lumen, Virtual Shadow Maps,
and high-cost quality settings for that process only.
