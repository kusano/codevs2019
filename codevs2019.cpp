#include <iostream>
#include <cassert>
using namespace std;

#include "ai_random.h"
#include "ai_chainer.h"

//  test.cpp
void test();

void readInitial(Status *status);
void readTurn(Status *status);

int main()
{
    //test();
    //return 0;

    cout<<"A.L.I.C.E."<<endl;

    Status status;
    //AIRandom ai;
    AIChainer ai;

    readInitial(&status);
    ai.initialize(status);

    for (int turn=0; turn<Status::maxTurn; turn++)
    {
        readTurn(&status);
        Move move = ai.think(status);

        if (move.bomb)
            cout<<"S"<<endl;
        else
            cout<<move.pos<<" "<<move.rotate<<endl;
    }
}

//  ゲーム開始時の読み込み
void readInitial(Status *status)
{
    for (int turn=0; turn<Status::maxTurn; turn++)
    {
        for (int i=0; i<4; i++)
        {
            int tmp;
            cin>>tmp;
            status->packs[turn][i] = (char)tmp;
        }
        string end;
        cin>>end;
        assert(end=="END");
    }
}

//  ターンごとの読み込み
void readTurn(Status *status)
{
    cin>>status->turn;
    for (int i=0; i<2; i++)
    {
        Field &f = status->fields[i];
        cin>>f.time;
        cin>>f.ojama;
        cin>>f.skill;
        cin>>f.score;
        for (int y=0; y<16; y++)
            for (int x=0; x<Field::W; x++)
            {
                int tmp;
                cin>>tmp;
                f.field[x][15-y] = (char)tmp;
            }
        for (int y=16; y<Field::H; y++)
            for (int x=0; x<Field::W; x++)
                f.field[x][y] = 0;
        string end;
        cin>>end;
        assert(end=="END");
    }
}
