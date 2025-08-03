.\build2022\Release\performance_test.exe --benchmark_min_time=1s --benchmark_format=json --benchmark_out=performance_results.json
python tools\analyze_benchmarks.py performance_results.json --output tools\test_report.txt
