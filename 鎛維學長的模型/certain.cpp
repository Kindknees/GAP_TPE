// 鎛維學長的確定性模型(論文3.2小節)

#include<iostream>
#include <set>
#include <string>
#include"gurobi_c++.h"
using namespace std;

int main()
{
	/*set*/
	set <int> ID;	//確定性航班集合
	set <int> IS;	//不確定性航班集合
	set <int> DD;

	/*params*/
	const int I = 4;	//所有航班數量
	const int K0 = 2;	//所有登機門，0為停機坪
	int A[I] = {400, 500, 600, 700};	//航班i的原訂抵達時間。
	int W = 50;	//更換登機門的懲罰值。
	int B = 100;	//指派至停機坪的懲罰值。
	int S[I] = {30, 45, 60, 50};	//航班i的服務時間。
	int U[I] = {450, 470, 480, 490};	//航班i的更新後抵達時間
	int Y[I][K0] = { {1, 0}, {0, 1}, {1, 0}, {0, 1} };	//航班i指派至登機門k的事前指派。
	int M = 1000;		//不知道這是什麼

	/*gurobi*/
	try
	{
		GRBEnv env = GRBEnv();
		GRBModel model = GRBModel(env);

		string name = "";

		/*variables*/
		GRBVar y[I][K0]; //若 = 1代表航班i使用登機門k
		GRBVar t[I];	//航班i在登機門的開始作業時間。
		GRBVar z[I];	//若=1代表航班i使用之登機門與事前指派不同

		//y[i][k], 若=1代表航班i使用登機門k
		for (int i = 0; i < I; i++)
		{
			for (int j = 0; j < K0; j++)
			{
				name = "y_" + to_string(i) + "_" + to_string(j);
				y[i][j] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
			}
		}
		cout << "y sucess" << endl;

		//t[i], 航班i在登機門的開始作業時間。
		for (int i = 0; i < I; i++)
		{
			name = "t_" + to_string(i);
			t[i] = model.addVar(0.0, GRB_INFINITY, 0, GRB_CONTINUOUS, name);
		}
		cout << "t success" << endl;
			
		//z[i], 若=1代表航班i使用之登機門與事前指派不同
		for (int i = 0; i < I; i++)
		{
			name = "z_" + to_string(i);
			z[i] = model.addVar(0.0, 1.0, 0, GRB_BINARY, name);
		}
		cout << "z success" << endl;

		GRBLinExpr sum = 0;


		/*ojective*/
		for (int i = 0; i < I; i++)
			sum += t[i] - A[i] + W*z[i] + B*y[i][0];
			
		model.setObjective(sum, GRB_MINIMIZE);

		/*constraints*/
		//號碼對應到論文
		//(2)
		for (int i = 0; i < I; i++)
		{
			sum = 0;
			for (int k = 0; k < K0; k++)
			{
				sum += y[i][k];
			}
			name = "(2)_" + to_string(i);
			model.addConstr(sum == 1, name);
		}
		cout << "(2) sucess" << endl;
		
		//(3)
		for (int i = 0; i < I; i++)
		{
			for (int j = 0; j < I; j++)
			{
				if (U[i] < U[j])
				{
					for (int k = 0; k < K0; k++)
					{
						name = "(3)_" + to_string(i) + "_" + to_string(j) + "_" + to_string(k);
						model.addConstr(t[i] + S[i] - t[j] <= (2 - y[i][k] - y[j][k]) * M, name);
					}
				}
			}
		}
		cout << "(3) success" << endl;
		
		//(4)
		for (int i = 0; i < I; i++)
		{
			name = "(4)" + to_string(i);
			model.addConstr(U[i] <= t[i], name);
		}
		cout << "(4) success" << endl;
		
		//(5)
		for (int i = 0; i < I; i++)
		{
			sum = 0;
			for (int k = 0; k < K0; k++)
			{
				sum += y[i][k] * (1 - Y[i][k]);
			}
			name = "(5) success" + to_string(i);
			model.addConstr(z[i] >= sum, name);
		}
		cout << "(5) success" << endl;



		model.optimize();

		//輸出y
		cout << "y:" << endl;
		for (int i = 0; i < I; i++)
		{
			for (int j = 0; j < K0; j++)
			{
				if (y[i][j].get(GRB_DoubleAttr_X) == 1)
				{
					cout <<"flight" + to_string(i + 1) + "-->gate " + to_string(j) << endl;
				}
			}
		}

		//輸出t
		cout << "t:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "flight" + to_string(i + 1) + ":" << to_string(t[i].get(GRB_DoubleAttr_X)) << endl;
		}

		//輸出z
		cout << "z:" << endl;
		for (int i = 0; i < I; i++)
		{
			cout << "flight" + to_string(i + 1) + ":" << to_string(z[i].get(GRB_DoubleAttr_X)) << endl;
		}
	}
	catch (GRBException e) {
		cout << "Error code = " << e.getErrorCode() << endl;
		cout << e.getMessage() << endl;
	}
	catch (...) {
		cout << "Exception during optimization" << endl;
	}

	system("pause");
	return 0;
}
