#include <iostream>
#include <cassert>
using namespace std;

#include "ai_random.h"
#include "ai_chainer.h"
#include "ai_alice.h"

//  test.cpp
void test();

void readInitial(Game *game);
void readTurn(Game *game);

int main()
{
    //test();
    //return 0;

    cout<<"A.L.I.C.E."<<endl;

    Game game;
    //AIRandom ai;
    //AIChainer ai;
    AIAlice ai;

    readInitial(&game);
    ai.initialize(game);

    for (int turn=0; turn<Game::MaxTurn; turn++)
    {
        readTurn(&game);
        Move move = ai.think(game);

        if (move.bomb)
            cout<<"S"<<endl;
        else
            cout<<move.pos<<" "<<move.rotate<<endl;
    }
}

//  ゲーム開始時の読み込み
void readInitial(Game *game)
{
    for (int turn=0; turn<Game::MaxTurn; turn++)
    {
        for (int i=0; i<4; i++)
        {
            int tmp;
            cin>>tmp;
            game->packs[turn][i] = (char)tmp;
        }
        string end;
        cin>>end;
        assert(end=="END");
    }
}

//  ターンごとの読み込み
void readTurn(Game *game)
{
    cin>>game->turn;
    for (int i=0; i<2; i++)
    {
        Field &f = game->fields[i];
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
