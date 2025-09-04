import pandas as pd
from io import StringIO
import numpy as np

# --- Your CSV Data ---
csv_data = """
ID,Mean Performance Factor,Standard Deviation Performance Factor,Base C++ Runtime,Improved C++ Runtime,C++ Timeouts,Minimal Improvement,
1,1,0,4.969,4.969,146,,
2,12.758,6.427,6.257,0.972,37,0.292,
3,1.487,1.144,6.305,4.99,132,1,
4,22.533,8.553,6.255,0.978,32,0.276,
5,17.596,4.936,6.29,0.405,12,0.588,
6,3.092,1.418,6.305,1.489,17,1,
7,12.022,5.672,6.29,0.586,13,0.394,?
8,16.581,1.813,6.305,0.298,2,1,?
9,24.15,22.89,6.305,1.282,34,1,
10,3.231,1.885,6.305,2.654,73,1,
11,39.33,22.586,6.305,0.873,29,1,
12,2.992,2.045,6.305,2.834,78,1,
13,28.204,4.284,6.305,0.2,5,0.885,
14,135.874,15.065,6.305,0.214,4,1,
15,4.607,1.96,6.305,1.129,21,1,
16,161.094,11.978,6.305,0.134,4,1,
"""
df_csv = pd.read_csv(StringIO(csv_data))

# --- Your Fractional Factorial Design Matrix ---
design_matrix_data = """
Test run,(1),(2),(3),(4),(5),(6),(7),(8)
1,-1,-1,-1,-1,-1,-1,-1,-1
2,+1,-1,-1,-1,-1,+1,+1,+1
3,-1,+1,-1,-1,+1,-1,+1,+1
4,+1,+1,-1,-1,+1,+1,-1,-1
5,-1,-1,+1,-1,+1,+1,+1,-1
6,+1,-1,+1,-1,+1,-1,-1,+1
7,-1,+1,+1,-1,-1,+1,-1,+1
8,+1,+1,+1,-1,-1,-1,+1,-1
9,-1,-1,-1,+1,+1,+1,-1,+1
10,+1,-1,-1,+1,+1,-1,+1,-1
11,-1,+1,-1,+1,-1,+1,+1,-1
12,+1,+1,-1,+1,-1,-1,-1,+1
13,-1,-1,+1,+1,-1,-1,+1,+1
14,+1,-1,+1,+1,-1,+1,-1,-1
15,-1,+1,+1,+1,+1,-1,-1,-1
16,+1,+1,+1,+1,+1,+1,+1,+1
"""
df_design = pd.read_csv(StringIO(design_matrix_data))

# Rename 'Test run' in design matrix to 'ID' for merging
df_design = df_design.rename(columns={'Test run': 'ID'})

# Merge the design matrix with your performance data
df_merged = pd.merge(df_csv, df_design, on='ID')

# Define your response variables
response_variables = {
    'Number of Timeouts': 'C++ Timeouts',
    'Average Improved C++ Runtime (s)': 'Improved C++ Runtime',
    'Mean Performance Factor': 'Mean Performance Factor',
    'Std. Dev. Performance Factor': 'Standard Deviation Performance Factor'
}

# Define your factors (the columns from your design matrix)
factors = [f'({i})' for i in range(1, 9)]

# Function to calculate the main effect of each factor
def calculate_main_effects(dataframe, response_column, factor_columns):
    effects = {}
    for factor in factor_columns:
        avg_high = dataframe[dataframe[factor] == 1][response_column].mean()
        avg_low = dataframe[dataframe[factor] == -1][response_column].mean()
        effects[factor] = avg_high - avg_low
    return effects

# Calculate effects for all response variables
all_effects = {}
for display_name, col_name in response_variables.items():
    all_effects[display_name] = calculate_main_effects(df_merged, col_name, factors)

# Prepare data for LaTeX table
latex_rows = []
for display_name, effects_dict in all_effects.items():
    row_values = [display_name] + [f"{effects_dict[factor]:.3f}" for factor in factors]
    latex_rows.append(' & '.join(row_values))

# Print the LaTeX table
print("\n--- LaTeX Table Output ---")
print(r"\begin{table}[h!]")
print(r"    \centering")
print(r"    \caption{Main Effects of 8 Factors on Performance Metrics}")
print(r"    \label{tab:main_effects_performance}")
print(r"    \begin{tabularx}{\textwidth}{l *{8}{S[table-format=-2.3]}}")
print(r"        \toprule")
print(r"        \textbf{Response Variable} & \multicolumn{8}{c}{\textbf{Factor}} \\")
print(r"        \cmidrule(lr){2-9}")
print(r"        & \textbf{(1)} & \textbf{(2)} & \textbf{(3)} & \textbf{(4)} & \textbf{(5)} & \textbf{(6)} & \textbf{(7)} & \textbf{(8)} \\")
print(r"        \midrule")
for row in latex_rows:
    print(f"        {row} \\\\")
print(r"        \bottomrule")
print(r"    \end{tabularx}")
print(r"\end{table}")