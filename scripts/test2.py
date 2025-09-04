import pandas as pd
import statsmodels.api as sm
from statsmodels.formula.api import ols
import io

# --- 1. Load the data ---

# Your performance data (as a string, mimicking reading from a CSV file)
performance_data_csv = """ID,Mean Performance Factor,Standard Deviation Performance Factor,Base C++ Runtime,Improved C++ Runtime,C++ Timeouts,Minimal Improvement,
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

# Your fractional factorial design (as a string)
design_data_csv = """Test run,(1),(2),(3),(4),(5),(6),(7),(8)
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

# Read into pandas DataFrames
df_performance = pd.read_csv(io.StringIO(performance_data_csv))
df_design = pd.read_csv(io.StringIO(design_data_csv))

# --- 2. Prepare and Merge DataFrames ---

# Rename 'ID' column in performance_data to 'Test run' for merging
df_performance = df_performance.rename(columns={'ID': 'Test run'})

# Rename columns with spaces to use underscores for compatibility with patsy/statsmodels formulas
df_performance.columns = df_performance.columns.str.replace(' ', '_')
df_performance.columns = df_performance.columns.str.replace('C\+\+', 'Cpp', regex=True) # Replace C++ with Cpp

# Merge the two DataFrames on 'Test run'
df_merged = pd.merge(df_performance, df_design, on='Test run')

# Clean up factor column names: remove parentheses and convert to string for formula API
factor_cols = [f'Factor{i}' for i in range(1, 9)]
for i in range(1, 9):
    original_col_name = f'({i})'
    df_merged = df_merged.rename(columns={original_col_name: factor_cols[i-1]})
    # Convert factor levels to categorical type for proper ANOVA handling
    df_merged[factor_cols[i-1]] = df_merged[factor_cols[i-1]].astype('category')


# Define response variables with the updated names
response_vars = ['Mean_Performance_Factor', 'Cpp_Timeouts', 'Improved_Cpp_Runtime']

print("--- Merged Data Head ---")
print(df_merged.head())
print("\n")

# --- 3. Perform ANOVA for each response variable ---

for response_var in response_vars:
    print(f"--- ANOVA for: {response_var} ---")

    # Construct the formula string for the linear model
    # C() around each factor explicitly tells statsmodels to treat it as a categorical variable
    # This is important for factorial designs where levels are often coded numerically (-1, +1)
    formula = f'{response_var} ~ ' + ' + '.join([f'C({col})' for col in factor_cols])

    # Fit the Ordinary Least Squares (OLS) model
    model = ols(formula, data=df_merged).fit()

    # Generate the ANOVA table (Type 2 sums of squares recommended for main effects in factorial designs)
    anova_table = sm.stats.anova_lm(model, typ=2)

    # Print the ANOVA table, focusing on p-values (PR(>F))
    print(anova_table[['df', 'sum_sq', 'F', 'PR(>F)']])
    print("\n")

    # Optional: Print model summary for more details (R-squared, coefficients, etc.)
    # print(model.summary())
    # print("\n")

print("--- Analysis Complete ---")
print("Interpretation of PR(>F) column (p-value):")
print("A p-value <= 0.05 generally indicates statistical significance (reject the null hypothesis of no effect).")
print("A p-value > 0.05 generally indicates no statistical significance (fail to reject the null hypothesis).")
print("Note: The design is Resolution IV, meaning main effects are not aliased with two-factor interactions,")
print("but may be aliased with three-factor or higher interactions. This is inherent to the design.")
