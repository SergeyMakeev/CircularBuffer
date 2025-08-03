#!/usr/bin/env python3
"""
Performance Benchmark Analysis Tool for CircularBuffer (ASCII version)

Analyzes benchmark JSON output and generates comprehensive performance comparisons
between CircularBuffer and std::deque across various operations.
"""

import json
import sys
import argparse
from typing import Dict, List, Tuple, Optional
from dataclasses import dataclass
from collections import defaultdict
import statistics

@dataclass
class BenchmarkResult:
    """Represents a single benchmark result."""
    name: str
    container: str
    operation: str
    size: Optional[int]
    cpu_time: float  # nanoseconds
    real_time: float  # nanoseconds
    items_per_second: Optional[float]
    iterations: int

class BenchmarkAnalyzer:
    """Analyzes benchmark results and generates performance comparisons."""
    
    def __init__(self, json_file: str):
        """Initialize with benchmark JSON file."""
        with open(json_file, 'r') as f:
            self.data = json.load(f)
        
        self.results = self._parse_benchmarks()
        self.operations = self._group_by_operation()
        
    def _parse_benchmarks(self) -> List[BenchmarkResult]:
        """Parse benchmark JSON into structured results."""
        results = []
        
        for bench in self.data.get('benchmarks', []):
            name = bench['name']
            
            # Parse name to extract operation and size
            parts = name.split('/')
            if len(parts) == 2:
                # Has size: "CircularBuffer_PushBack/1000"
                size = int(parts[1])
                operation = parts[0].split('_', 1)[1]  # Remove container prefix
            elif len(parts) == 1:
                # No size: "CircularBuffer_Construction"
                size = None
                operation = parts[0].split('_', 1)[1]  # Remove container prefix
            else:
                continue  # Skip malformed names
            
            # Extract container type
            if name.startswith('CircularBuffer_'):
                container = 'CircularBuffer'
            elif name.startswith('StdDeque_'):
                container = 'std::deque'
            elif name.startswith('StdVector_'):
                container = 'std::vector'
            else:
                continue  # Skip unknown containers
            
            result = BenchmarkResult(
                name=name,
                container=container,
                operation=operation,
                size=size,
                cpu_time=bench['cpu_time'],
                real_time=bench['real_time'],
                items_per_second=bench.get('items_per_second'),
                iterations=bench['iterations']
            )
            results.append(result)
        
        return results
    
    def _group_by_operation(self) -> Dict[str, Dict[str, List[BenchmarkResult]]]:
        """Group results by operation and container."""
        operations = defaultdict(lambda: defaultdict(list))
        
        for result in self.results:
            operations[result.operation][result.container].append(result)
        
        return dict(operations)
    
    def generate_summary(self) -> str:
        """Generate comprehensive performance summary."""
        report = []
        report.append("=" * 80)
        report.append("CIRCULAR BUFFER PERFORMANCE ANALYSIS")
        report.append("=" * 80)
        report.append("")
        
        # System info
        context = self.data.get('context', {})
        report.append(f"System: {context.get('num_cpus', 'N/A')} CPUs @ {context.get('mhz_per_cpu', 'N/A')} MHz")
        report.append(f"Date: {context.get('date', 'N/A')}")
        report.append(f"Build: {context.get('library_build_type', 'N/A')}")
        report.append("")
        
        # Operation-by-operation analysis
        for operation in sorted(self.operations.keys()):
            report.extend(self._analyze_operation(operation))
            report.append("")
        
        # Overall summary
        report.extend(self._generate_overall_summary())
        
        return "\n".join(report)
    
    def _analyze_operation(self, operation: str) -> List[str]:
        """Analyze performance for a specific operation."""
        results = []
        results.append(f"[ANALYSIS] {operation.upper()}")
        results.append("-" * 60)
        
        containers = self.operations[operation]
        
        # Group by size for comparison
        size_comparisons = defaultdict(dict)
        for container, benchmarks in containers.items():
            for bench in benchmarks:
                if bench.size is not None:
                    size_comparisons[bench.size][container] = bench
        
        if not size_comparisons:
            # Handle operations without sizes (like Construction)
            results.append("Single benchmark comparison (iterations):")
            fastest_container = None
            highest_iterations = 0
            
            for container, benchmarks in containers.items():
                if benchmarks:
                    bench = benchmarks[0]  # Take first (should be only one)
                    iterations = bench.iterations
                    results.append(f"  {container:15}: {iterations:,} iter")
                    
                    if bench.iterations > highest_iterations:
                        highest_iterations = bench.iterations
                        fastest_container = container
            
            if fastest_container:
                results.append(f"  [WINNER]: {fastest_container}")
        else:
            # Compare across different sizes
            results.append("Performance by input size (iterations completed):")
            results.append(f"{'Size':<8} {'CircularBuffer':<15} {'std::deque':<15} {'Winner':<15}")
            results.append("-" * 60)
            
            for size in sorted(size_comparisons.keys()):
                size_data = size_comparisons[size]
                
                # Get iterations and find winner (higher iterations = better performance)
                iterations = {}
                for container, bench in size_data.items():
                    iterations[container] = bench.iterations
                
                # Find fastest (highest iterations)
                if iterations:
                    winner = max(iterations, key=iterations.get)
                    
                    # Format row
                    row = f"{size:<8}"
                    for container in ['CircularBuffer', 'std::deque']:
                        if container in iterations:
                            iter_str = f"{iterations[container]:,} iter"
                            if container == winner:
                                iter_str = f"*{iter_str}*"
                        else:
                            iter_str = "N/A"
                        row += f" {iter_str:<15}"
                    
                    results.append(row)
            
            # Performance ratios for largest size
            largest_size = max(size_comparisons.keys())
            if len(size_comparisons[largest_size]) >= 2:
                results.append("")
                results.append(f"Performance ratios at size {largest_size}:")
                self._add_performance_ratios(results, size_comparisons[largest_size])
        
        return results
    
    def _add_performance_ratios(self, results: List[str], benchmarks: Dict[str, BenchmarkResult]):
        """Add performance ratio comparisons based on iterations."""
        if 'CircularBuffer' not in benchmarks:
            return
        
        cb_iterations = benchmarks['CircularBuffer'].iterations
        
        for container, bench in benchmarks.items():
            if container != 'CircularBuffer':
                ratio = bench.iterations / cb_iterations
                if ratio > 1:
                    results.append(f"  {container} is {ratio:.2f}x faster than CircularBuffer")
                else:
                    results.append(f"  CircularBuffer is {1/ratio:.2f}x faster than {container}")
    
    def _generate_overall_summary(self) -> List[str]:
        """Generate overall performance summary."""
        results = []
        results.append("[OVERALL PERFORMANCE SUMMARY]")
        results.append("=" * 60)
        
        # Count wins by container
        wins = defaultdict(int)
        total_comparisons = 0
        
        for operation, containers in self.operations.items():
            # Group by size and find winners
            size_comparisons = defaultdict(dict)
            for container, benchmarks in containers.items():
                for bench in benchmarks:
                    key = bench.size if bench.size is not None else 'single'
                    size_comparisons[key][container] = bench
            
            for size_data in size_comparisons.values():
                if len(size_data) >= 2:  # Need at least 2 containers to compare
                    fastest = max(size_data, key=lambda c: size_data[c].iterations)
                    wins[fastest] += 1
                    total_comparisons += 1
        
        if total_comparisons > 0:
            results.append(f"Performance wins out of {total_comparisons} comparisons:")
            for container in sorted(wins.keys(), key=wins.get, reverse=True):
                percentage = (wins[container] / total_comparisons) * 100
                results.append(f"  {container:<15}: {wins[container]:3d} wins ({percentage:5.1f}%)")
        
        results.append("")
        results.append("KEY INSIGHTS:")
        
        # CircularBuffer strengths
        cb_strengths = []
        cb_weaknesses = []
        
        for operation, containers in self.operations.items():
            if 'CircularBuffer' in containers and len(containers) >= 2:
                # Find if CircularBuffer is fastest for this operation (average across sizes)
                cb_avg_iterations = self._get_average_iterations(containers['CircularBuffer'])
                other_avg_iterations = [
                    self._get_average_iterations(benchmarks) 
                    for container, benchmarks in containers.items() 
                    if container != 'CircularBuffer'
                ]
                
                if cb_avg_iterations and other_avg_iterations:
                    if cb_avg_iterations > max(other_avg_iterations):
                        cb_strengths.append(operation)
                    elif cb_avg_iterations < min(other_avg_iterations):
                        cb_weaknesses.append(operation)
        
        if cb_strengths:
            results.append(f"  [+] CircularBuffer excels at: {', '.join(cb_strengths)}")
        if cb_weaknesses:
            results.append(f"  [-] CircularBuffer slower at: {', '.join(cb_weaknesses)}")
        
        # Memory efficiency note
        results.append("  [*] CircularBuffer offers predictable memory usage (no dynamic allocation)")
        results.append("  [*] CircularBuffer provides O(1) push_front operations (same as std::deque)")
        results.append("  [*] CircularBuffer maintains consistent performance with wraparound")
        
        return results
    
    def _get_average_iterations(self, benchmarks: List[BenchmarkResult]) -> Optional[float]:
        """Get average iterations for a list of benchmarks."""
        if not benchmarks:
            return None
        return statistics.mean(bench.iterations for bench in benchmarks)
    
    def export_csv(self, filename: str):
        """Export results to CSV for further analysis."""
        import csv
        
        with open(filename, 'w', newline='') as csvfile:
            writer = csv.writer(csvfile)
            writer.writerow(['Operation', 'Container', 'Size', 'CPU_Time_ns', 'Real_Time_ns', 
                           'Items_Per_Second', 'Iterations'])
            
            for result in self.results:
                writer.writerow([
                    result.operation, result.container, result.size or '',
                    result.cpu_time, result.real_time, 
                    result.items_per_second or '', result.iterations
                ])

def main():
    parser = argparse.ArgumentParser(description='Analyze circular buffer benchmark results')
    parser.add_argument('json_file', help='Path to benchmark JSON results file')
    parser.add_argument('--csv', help='Export results to CSV file')
    parser.add_argument('--output', help='Output summary to file')
    
    args = parser.parse_args()
    
    try:
        analyzer = BenchmarkAnalyzer(args.json_file)
        summary = analyzer.generate_summary()
        
        if args.output:
            with open(args.output, 'w', encoding='utf-8') as f:
                f.write(summary)
            print(f"Summary written to {args.output}")
        else:
            print(summary)
        
        if args.csv:
            analyzer.export_csv(args.csv)
            print(f"CSV data exported to {args.csv}")
            
    except FileNotFoundError:
        print(f"Error: File '{args.json_file}' not found", file=sys.stderr)
        sys.exit(1)
    except json.JSONDecodeError:
        print(f"Error: Invalid JSON in '{args.json_file}'", file=sys.stderr)
        sys.exit(1)
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == '__main__':
    main() 