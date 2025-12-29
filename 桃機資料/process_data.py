"""
將桃機資料進行處理，例如：轉換時間格式、處理跨日問題、計算service time等。
"""


import pandas as pd

def process_flight_data(input_filename, output_filename):
    try:
        df = pd.read_csv(input_filename, dtype=str)
    except FileNotFoundError:
        print(f"錯誤：找不到檔案 '{input_filename}'")
        return

    df.insert(0, 'flight_index', range(1, len(df) + 1))

    def time_str_to_minutes(time_str):
        if pd.isna(time_str) or str(time_str).strip() == '':
            return None
        try:
            time_int = int(float(time_str))
            time_str_formatted = str(time_int).zfill(4)
            hours = int(time_str_formatted[:2])
            minutes = int(time_str_formatted[2:])
            return hours * 60 + minutes
        except (ValueError, TypeError):
            return None

    df['start_time_min'] = df['start_time_str'].apply(time_str_to_minutes)
    df['end_time_min'] = df['end_time_str'].apply(time_str_to_minutes)

    df['updated_start_min'] = df['updated_start_str'].apply(time_str_to_minutes)
    df['updated_end_min'] = df['updated_end_str'].apply(time_str_to_minutes)

    # 處理跨日問題
    # 如果結束時間小於開始時間，則將結束時間加上 24 小時（1440 分鐘）
    
    # original time
    original_valid_times = df['start_time_min'].notna() & df['end_time_min'].notna()
    original_cross_day = original_valid_times & (df['end_time_min'] < df['start_time_min'])
    df.loc[original_cross_day, 'end_time_min'] += 1440

    # updated time
    updated_valid_times = df['updated_start_min'].notna() & df['updated_end_min'].notna()
    updated_cross_day = updated_valid_times & (df['updated_end_min'] < df['updated_start_min'])
    df.loc[updated_cross_day, 'updated_end_min'] += 1440

    df['service_time_min'] = df['end_time_min'] - df['start_time_min']

    df["start_time_min"] = df["start_time_min"].astype('Int64')
    df["end_time_min"] = df["end_time_min"].astype('Int64')
    df["updated_start_min"] = df["updated_start_min"].astype('Int64')
    df["updated_end_min"] = df["updated_end_min"].astype('Int64')
    df["service_time_min"] = df["service_time_min"].astype('Int64')
    df.to_csv(output_filename, index=False, encoding='utf_8_sig')
    
    print(f"資料處理完成！更新後的資料已儲存至 '{output_filename}'")

input_file = "0623_ver81.csv"
output_file = "processed_0623_ver81.csv"
process_flight_data(input_file, output_file)