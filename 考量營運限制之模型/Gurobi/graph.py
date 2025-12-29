import pandas as pd
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.font_manager as fm
import numpy as np

"""
讀取結果的 CSV 檔案，並產生甘特圖。
"""
try:
    df = pd.read_csv('results.csv')
except FileNotFoundError:
    print("cannot find gate_assignment_results.csv")
    exit()

# --- 1. 資料前處理 ---
# 從 '最新指派' 欄位中提取數字，以便排序 (停機坪設為 0)
df['gate_numeric'] = df['最新指派'].apply(lambda x: int(x.split(' ')[-1]) if '登機門' in x else 0)
# 根據登機門號碼排序，確保 Y 軸順序正確
df = df.sort_values(by='gate_numeric', ascending=True)

# --- 2. 設定中文字體 ---
# 為了在圖中正確顯示中文，需要設定字體。
# 請根據您的作業系統選擇一個可用的中文字體。
# Windows: 'Microsoft JhengHei' (微軟正黑體), 'SimHei'
# macOS: 'Arial Unicode MS', 'PingFang TC'
# Linux: 'WenQuanYi Zen Hei', 'Noto Sans CJK TC'

plt.rcParams['font.sans-serif'] = ['Heiti TC']  # MacOS 內建中文字型
plt.rcParams['axes.unicode_minus'] = False  # 正確顯示負號


# --- 3. 繪製甘特圖 ---
fig, ax = plt.subplots(figsize=(20, 10))

# 取得所有使用到的登機門/停機坪的數字編號並排序
gate_numbers = sorted(df['gate_numeric'].unique())
gate_to_y_pos = {gate: i for i, gate in enumerate(gate_numbers)}
y_pos = np.arange(len(gate_numbers))

unique_flights = df['航班ID'].unique()
num_flights = len(unique_flights)

cmap = matplotlib.colormaps.get_cmap('viridis') 
colors_list = cmap(np.linspace(0, 1, num_flights))

flight_colors = {flight_id: colors_list[i] for i, flight_id in enumerate(unique_flights)}

for index, task in df.iterrows():
    start_time = task['作業開始時間']
    end_time = task['作業結束時間']
    duration = end_time - start_time
    
    gate_num = task['gate_numeric']
    y = gate_to_y_pos[gate_num]
    
    ax.barh(
        y, 
        duration, 
        left=start_time, 
        height=0.6, 
        align='center',
        color=flight_colors[task['航班ID']],
        edgecolor='black'
    )
    
    # 在長條中間加上航班 ID 文字
    text_color = 'white' if sum(flight_colors[task['航班ID']][:3]) < 1.5 else 'black'
    ax.text(
        start_time + duration / 2, 
        y, 
        f"{task['航班ID']}", 
        ha='center', 
        va='center',
        color=text_color,
        fontweight='bold'
    )

ax.set_yticks(y_pos)
ax.set_yticklabels(gate_numbers, fontsize=12)
ax.invert_yaxis()  # 讓 y=0 (停機坪) 在最下面

ax.set_xlabel('Time(mins)', fontsize=14)
ax.set_ylabel('Gate', fontsize=14)
ax.set_title('GAP problem', fontsize=18, fontweight='bold')

max_time = df['作業結束時間'].max()
ax.set_xticks(np.arange(0, max_time + 60, 60))
ax.tick_params(axis='x', rotation=45)

ax.grid(True, which='major', axis='x', linestyle='--', linewidth=0.5)

plt.tight_layout()


output_filename = 'result.png'
plt.savefig(output_filename, dpi=300)
print(f"甘特圖已成功儲存至檔案: {output_filename}")