#include <iostream>
#include <random>
#include <cstring>
#include <chrono>
#include <cassert>
#include "game.h"
#include "ai_alice.h"
#include "table.h"
using namespace std;

void test1();
void test2();
void test3();
void setField(Field *field, char f[16][10]);

void test()
{
    test1();
    //test2();
    test3();
}

void test1()
{
    //  Field::move/undo/candChain
    Field field;

    char old[Game::MaxTurn][Field::W][Field::H];
    unsigned long long oldHash[Game::MaxTurn];
    mt19937 random;
    for (int i=0; i<254; i++)
    {
        memcpy(old[i], field.field, sizeof old[i]);
        oldHash[i] = field.hash;
        field.candChain();
        assert(memcmp(old[i], field.field, sizeof old[i])==0);
        assert(oldHash[i]==field.hash);

        field.move(Move(random()%9, random()%4, false), samplePacks[i]);
        assert(i==253 || !field.isDead());
    }

    assert(field.isDead());
    field.undo();
    assert(!field.isDead());
    assert(memcmp(old[253], field.field, sizeof old[253])==0);
    assert(oldHash[253]==field.hash);

    field.move(Move(0, 0, true), nullptr);

    for (int i=253; i>=0; i--)
    {
        field.undo();
        assert(memcmp(old[i], field.field, sizeof old[i])==0);
        assert(oldHash[i]==field.hash);
    }
}

void test2()
{
    Game game;
    memcpy(game.packs, samplePacks, sizeof game.packs);

    AIAlice ai;
    ai.initialize(game, 0);

    chrono::system_clock::time_point start = chrono::system_clock::now();

    game.fields[0].time = 180000;
    Move move = ai.think(game);

    chrono::system_clock::time_point end = chrono::system_clock::now();
    double duration =
        chrono::duration_cast<chrono::nanoseconds>(end-start).count() * 1e-9;
    cout<<duration<<endl;
}

void test3()
{
    //  candChain

    //  お邪魔ブロックによって連鎖が可能になる
    char f1[16][10] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {5, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {7, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {8, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {9,11, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    //  お邪魔ブロックによって連鎖が不可能になる
    char f2[16][10] =
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {2, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {3, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {5,11, 0, 0, 0, 0, 0, 0, 0, 0},
        {7,11, 0, 0, 0, 0, 0, 0, 0, 0},
        {8,11, 0, 0, 0, 0, 0, 0, 0, 0},
        {9,11, 0, 0, 0, 0, 0, 0, 0, 0},
    };

    Field field;
    setField(&field, f1);
    assert(field.candChain()==1);
    field.ojama = 12;
    assert(field.candChain()==4);

    setField(&field, f2);
    field.ojama = 9;
    assert(field.candChain()==4);
    field.ojama = 34;
    assert(field.candChain()==1);
}

void setField(Field *field, char f[16][10])
{
    for (int x=0; x<Field::W; x++)
    {
        for (int y=0; y<16; y++)
            field->field[x][y] = f[15-y][x];
        for (int y=16; y<Field::H; y++)
            field->field[x][y] = 0;
    }
}
