# 鎛維學長的確定性模型(論文3.2小節)
import gurobipy as gp
from gurobipy import GRB

import gurobipy as gp
from gurobipy import GRB
import pandas as pd
import traceback

# --- Sets ---

df_flights = pd.read_csv("./processed_0623_ver81.csv", encoding="utf-8-sig", dtype=str)
gates = [i for i in range (1, 38, 1)]   # 登機門 K 的集合
apron = [i for i in range (38, 53, 1)]               # 停機坪 (以 0 代表)
locations = apron + gates # 登機門(包含停機坪) K^0 的集合

K = gates
K0 = locations

I = len(df_flights)
flights = []
A=dict()
S=dict()
U=dict()
Y = dict()  # Pre-assigned gates

for _, row in df_flights.iterrows():
  if pd.isna(row["service_time_min"]):
    continue
  flights.append(int(row["flight_index"]))
  A[int(row["flight_index"])] = int(row["start_time_min"])
  S[int(row["flight_index"])] = int(row["service_time_min"])
  U[int(row["flight_index"])] = int(row["updated_start_min"])
  Y[int(row["flight_index"])] = int(row["original_gate_index"])

W = 50  # Penalty for changing gate assignment
B = 100  # Penalty for assigning to apron
M = 100000  # Big-M constant for constraints


# try:
# Create Gurobi environment and model
env = gp.Env()
model = gp.Model("GateAssignment", env=env)

# Variables
# y[i][k]: 1 if flight i is assigned to gate k
y = {}
for i in flights:
    for k in K0:
        y[i,k] = model.addVar(vtype=GRB.BINARY, name=f"y_{i}_{k}")

# t[i]: start time of flight i at gate
t = {}
for i in flights:
    t[i] = model.addVar(vtype=GRB.CONTINUOUS, lb=0.0, name=f"t_{i}")

# z[i]: 1 if flight i's gate differs from pre-assignment
z = {}
for i in flights:
    z[i] = model.addVar(vtype=GRB.BINARY, name=f"z_{i}")

print("Variables created successfully")

# Objective: Minimize total delay + gate change penalties + apron penalties
obj = gp.quicksum(t[i] - A[i] + W*z[i] for i in flights) + B*gp.quicksum(y[i, k] for i in flights for k in apron)
model.setObjective(obj, GRB.MINIMIZE)

# Constraints
# (2)
for i in flights:
    model.addConstr(sum(y[i,k] for k in K0) == 1, name=f"constr_2_{i}")
print("Constraint (2) added")

# (3)
for i in flights:
    for j in flights:
        if U[i] < U[j]:
            for k in K0:
                model.addConstr(
                    t[i] + S[i] - t[j] <= (2 - y[i,k] - y[j,k]) * M,
                    name=f"constr_3_{i}_{j}_{k}"
                )
print("Constraint (3) added")

# (4)
for i in flights:
    model.addConstr(t[i] >= U[i], name=f"constr_4_{i}")
print("Constraint (4) added")

# (5)
for i in flights:
    model.addConstr(
        z[i] >= sum(y[i, k] * (1 - int(Y.get(i, -1) == k)) for k in K0),
        name=f"constr_5_{i}"
    )
print("Constraint (5) added")

# Optimize model
model.optimize()

# Output results
print("\ny:")
for i in flights:
    for k in K0:
        if y[i,k].X > 0.5:  # Check if binary variable is 1
            print(f"flight{i} --> gate {k}")

print("\nt:")
for i in flights:
    print(f"flight{i}: {t[i].X}")

print("\nz:")
for i in flights:
    print(f"flight{i}: {z[i].X}")