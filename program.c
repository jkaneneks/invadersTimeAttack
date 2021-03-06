#include "program.h"

bool gameOver = false;
int bounceCounter = 1;
int gunCooldown = 0;
Direction lastDirection = LEFT;
unsigned long long frameCounter = 0;
int globalScore = 0;

int main(void)
{    
    /* start InputThread, special thanks to j.smolka for support <3 */
    InputThread *inputThread = threadAlloc();
    threadStart(inputThread);
    
    SetUp();

    while (true)
    {
        if(1 == RunGame(inputThread))
        break;
    }

    free(inputThread);
    /* release ncurses */
    SetDown();

    /* Clear Terminal*/
    printf("\033[H\033[J");

    return EXIT_SUCCESS;
}


void SetUp()
{
    //ncurses options
    initscr();              /* Start ncurses mode */
    //raw();                /* disable line buffering - no Endline or CR needed */
    cbreak();               /* like raw, but enables CTRL+C */
    keypad(stdscr, TRUE);   /* enable keys like F1, arrowkeys ... */
    noecho();               /* disable input buffer on screen e.g. escape stuff*/
    curs_set(0);            /* disable cursor blinki blinki */
    //nodelay(stdscr, 1);   /* */
    srand(time(NULL));
}

void SetDown()
{
    ClearTerminal();
    refresh();
    endwin();              /* stop ncurses mode IMPORTANT! ;D*/
}

void Dispose(Player player, Invader invaders[], Projectile projectiles[], Bomb bombs[], Shield shields[])
{
    /* free game entities */
    for(int s = 0; s < _MaximumShields; s++)
    {
        free(shields[s].Position);
    }
    for(int i = 0; i < (_InvaderPerRow * _InvaderRowCount); i++)
    {
        free(invaders[i].Position);
    }
    for(int p = 0; p < _MaximumProjectiles; p++)
    {
        free(projectiles[p].Position);
    }
    for(int b = 0; b < _MaximumBombs; b++)
    {
        free(bombs[b].Position);
    }
    free(player.Position);
}

/* ================================================================================================================= */
/* ====================================================== DRAW ===================================================== */
/* ================================================================================================================= */
void DrawInvaders(Invader invaders[]) 
{
    int i = 0;
    while (i < (_InvaderPerRow * _InvaderRowCount))
    {
        if(!invaders[i].Health)
        {
            i++;
            continue;
        }

        switch (invaders[i].SymbolSwitch)
        {
            case ONE:
                mvaddch(invaders[i].Position->Row, invaders[i].Position->Column, invaders[i].SymbolOne);
                break;
            case TWO:
                mvaddch(invaders[i].Position->Row, invaders[i].Position->Column, invaders[i].SymbolTwo);
                break;
            case THREE:
                mvaddch(invaders[i].Position->Row, invaders[i].Position->Column, invaders[i].SymbolThree);
                break;
            case FOUR:
                mvaddch(invaders[i].Position->Row, invaders[i].Position->Column, invaders[i].SymbolFour);
                break;
        }
        i++;
    }
}

void DrawProjectiles(Projectile projectiles[])
{
    
    for (int i = 0 ; i < _MaximumProjectiles; i++)
    {
        if (projectiles[i].Collision == false)
        {
            mvaddch(projectiles[i].Position->Row, projectiles[i].Position->Column, projectiles[i].Symbol);
        }
        else
        {
            mvaddch(projectiles[i].Position->Row, projectiles[i].Position->Column, ' ');
            //DeleteChar(projectiles[i]->Position);
        }
    }
}

void DrawTime(int timeInSeconds)
{ 
    mvprintw(LINES - 2, 1, "<time: %d>", timeInSeconds);
}

void DrawShields(Shield shields[])
{
    int i = 0;
    while (i < _MaximumShields)
    {
        if (shields[i].Health > 0)
        {
            switch (shields[i].SymbolSwitch)
            {
                case ONE:
                    mvaddch(shields[i].Position->Row, shields[i].Position->Column, shields[i].SymbolOne);
                    break;
                case TWO:
                    mvaddch(shields[i].Position->Row, shields[i].Position->Column, shields[i].SymbolTwo);
                    break;
                case THREE:
                    mvaddch(shields[i].Position->Row, shields[i].Position->Column, shields[i].SymbolThree);
                    break;
                case FOUR:
                    mvaddch(shields[i].Position->Row, shields[i].Position->Column, shields[i].SymbolFour);
                    break;            
            }
        }
        else
        {
            mvaddch(shields[i].Position->Row, shields[i].Position->Column, ' ');
        }
        i++;
    }
}

void DrawPlayer(Player player)
{
    mvaddch(player.Position->Row, player.Position->Column, player.Symbol);
}

void DeleteChar(Position *position)
{
    mvaddch(position->Row, position->Column, ' ');
}

/* ================================================================================================================= */
/* ====================================================== LOGIC ==================================================== */
/* ================================================================================================================= */

void GetNextPosition(Position *lastPosition, Position *newPosition, int listCount)
{
    /* first invader */
    if (lastPosition == NULL)
    {
        newPosition->Row = _InvaderFirstRow;
        newPosition->Column = _InvaderFirstColumn;
        return;
    }

    /* ohters */
    /* new row */
    if (listCount % _InvaderPerRow == 0)
    {
        newPosition->Row = lastPosition->Row + _InvaderVerticalSpace;
        newPosition->Column = _InvaderFirstColumn;
    }
    /* next column */
    else
    {
        newPosition->Row = lastPosition->Row;
        newPosition->Column = lastPosition->Column + _InvaderHorizontalSpace;
    }
}

void MoveInvaders(Invader invader[])
{    
    /* check and set direction */
    ValidateInvaderDirection(invader);   

    /* switch appearence */
    int i = 0;
    while (i < (_InvaderPerRow * _InvaderRowCount))
    {
        switch (invader[i].SymbolSwitch)
        {
            case ONE:
                invader[i].SymbolSwitch = THREE;
                break;
            case TWO:
                invader[i].SymbolSwitch = THREE;
                break;
            case THREE:
                invader[i].SymbolSwitch = FOUR;
                break;
            case FOUR:
                invader[i].SymbolSwitch = THREE;
                break;
        }

        /* delete old position */
        DeleteChar(invader[i].Position);

        /* move */
        switch (invader[i].Direction)
        {
            case LEFT:
                invader[i].Position->Column = invader[i].Position->Column - _MoveHorizontalStep;
                break;
            case RIGTH:
                invader[i].Position->Column = invader[i].Position->Column + _MoveHorizontalStep;
                break;
            case DOWN:
                invader[i].Position->Row++;
                invader[i].Direction = LEFT;
            break;
            default:
                break;
        }
        
        i++;
    }
}

void ValidateInvaderDirection(Invader invader[])
{
    int min = invader[0].Position->Column - _MoveHorizontalStep;
    int max = invader[0].Position->Column + _MoveHorizontalStep;

    int i = 0;
    while (i < (_InvaderPerRow * _InvaderRowCount))
    {
        if (invader[i].Direction == DOWN)
            invader[i].Direction = lastDirection;
        else
            lastDirection = invader[i].Direction;


        if (invader[i].Position->Column - _MoveHorizontalStep < min)
            min = invader[i].Position->Column - _MoveHorizontalStep;

        if (invader[i].Position->Column + _MoveHorizontalStep > max)
            max = invader[i].Position->Column + _MoveHorizontalStep;

        if (i++ > _InvaderPerRow)
            break;
        i++;
    }

    if ((min <= 1 && invader[i].Direction == LEFT)
    || (max >= COLS && invader[i].Direction == RIGTH))
    {
        bounceCounter++;
    }

    i = 0;
    while (i < (_InvaderPerRow * _InvaderRowCount))
    {
        if (min <= 1 && invader[i].Direction == LEFT)
        {
            invader[i].Direction = RIGTH;
        }
        if (max >= COLS && invader[i].Direction == RIGTH)
        {
            invader[i].Direction = LEFT;
        }

        if (bounceCounter >= 3)
        {
            invader[i].Direction = DOWN;
        }
        i++;
    }
    if (bounceCounter >= 3)    
        bounceCounter = 1;

}

void Shoot(Projectile projectiles[], Player player)
{
    int i = 0;
    while (i < _MaximumProjectiles)
    {
        if (projectiles[i].Collision == true)
        {
            if (projectiles[i].Position->Column == player.Position->Column
            && projectiles[i].Position->Row == player.Position->Row - 1)
            {
                break;
            }

            projectiles[i].Symbol = _ProjectileAppearence;
            projectiles[i].Position->Column = player.Position->Column;
            projectiles[i].Position->Row = player.Position->Row - 1;
            projectiles[i].Collision = false;
            
            break;
        }

        i++;
    }
}

void MoveProjectiles(Projectile projectiles[])
{
    for(int i = 0; i < _MaximumProjectiles ; i++)
    {
        if(projectiles[i].Collision == false) // solange projektile keine Collision haben, flieg
        {
            DeleteChar(projectiles[i].Position);
            projectiles[i].Position->Row--;     

            if (projectiles[i].Position->Row < 1)
                projectiles[i].Collision = true;
        }
    }
}

bool DetectCollision(Player *player, Invader invaders[], Projectile projectiles[], Bomb bombs[], Shield shields[], int timeInSeconds)
{
    bool allInvadersDefeated = true;
    for(int p = 0; p < _MaximumProjectiles; p++)
    {
        if (projectiles[p].Collision == true)
            continue;

        bool hit = false;
        /* collision with a shield */
        for(int s = 0; s < _MaximumShields; s++)
        {
            if (shields[s].Health <= 0)
                continue;

            if (shields[s].Position->Row == projectiles[p].Position->Row
            && shields[s].Position->Column == projectiles[p].Position->Column)
            {
                DealShieldDamage(&shields[s]);
                projectiles[p].Collision = true;
                projectiles[p].Symbol = _DisabledAppearence;
                DeleteChar(projectiles[p].Position);
                hit = true;
                break;
            }
        }

        if (hit)
        {
            continue;
        }

        /* collision with a bomb */
        for(int b = 0; b < _MaximumBombs; b++)
        {
            if (bombs[b].Collision == true)
                continue;
            
            if (bombs[b].Position->Row == projectiles[p].Position->Row
            && bombs[b].Position->Column == projectiles[p].Position->Column)
            {
                bombs[b].Collision = true;
                bombs[b].Symbol = _DisabledAppearence;
                projectiles[p].Collision = true;
                projectiles[p].Symbol = _DisabledAppearence;
                DeleteChar(bombs[b].Position);
                DeleteChar(projectiles[p].Position);
                hit = true;
                break;
            }

        }

        if (hit)
        {
            continue;
        }

        /* collision with a invader */
        for(int i = 0; i < (_InvaderPerRow * _InvaderRowCount); i++)
        {
            if (invaders[i].Health == false)
                continue;
            if (invaders[i].Position->Row == projectiles[p].Position->Row
            && invaders[i].Position->Column == projectiles[p].Position->Column)
            {
                player->Score += timeInSeconds;
                globalScore += timeInSeconds;
                invaders[i].Health = false;
                projectiles[p].Collision = true;
                DeleteChar(projectiles[p].Position);
                DeleteChar(invaders[i].Position);
            }
       
        }
    }

    for(int b = 0; b < _MaximumBombs; b++)
    {
        if (bombs[b].Collision == true)
            continue;

        bool hit = false;

        /* collision with a shield */
        for(int s = 0; s < _MaximumShields; s++)
        {
            if (shields[s].Health <= 0)
                continue;

            if (shields[s].Position->Row == bombs[b].Position->Row
            && shields[s].Position->Column == bombs[b].Position->Column)
            {
                DealShieldDamage(&shields[s]);
                bombs[b].Collision = true;
                bombs[b].Symbol = _DisabledAppearence;
                DeleteChar(bombs[b].Position);
                hit = true;
                break;
            }
        }

        if (hit)
        {
            continue;
        }

        /* collision with player */
        if (player->Position->Row == bombs[b].Position->Row
        && player->Position->Column == bombs[b].Position->Column)
        {
            player->Health--;
            if (player->Health <= 0)
                gameOver = true;

            bombs[b].Collision = true;
            bombs[b].Symbol = _DisabledAppearence;
            DeleteChar(bombs[b].Position);
            DrawPlayer((*player));
            continue;
        }

    }


    /* collision invader with shield */
    for(int i = 0; i < (_InvaderPerRow * _InvaderRowCount); i++)
    {
        if (invaders[i].Health == false)
            continue;

        allInvadersDefeated = false;

        for(int s = 0; s < _MaximumShields; s++)
        {
            if (shields[s].Health <= 0)
                continue;

            if (shields[s].Position->Row == invaders[i].Position->Row
            && shields[s].Position->Column == invaders[i].Position->Column)
            {
                shields[s].Health = 0;
                break;
            }
        }
    }

    return allInvadersDefeated;
}

void DealShieldDamage(Shield *shield)
{
    shield->Health--;

    switch (shield->Health)
    {
        case 4:
            shield->SymbolSwitch = ONE;
            break;
        case 3:
            shield->SymbolSwitch = TWO;
            break;
        case 2:
            shield->SymbolSwitch = THREE;
            break;
        case 1:
            shield->SymbolSwitch = FOUR;
            break;            
    }

    if (shield->Health == 0)
    {
        DeleteChar(shield->Position);
    }
}

void DrawScore(Player player)
{
    // mvprintw(LINES - 2, 1, "<score: %d>", player.Score);
    mvprintw(LINES - 2, ((int)COLS / 2) - 5, "<score: %d>", globalScore);
}

void DrawHealth(Player player)
{
    mvprintw(LINES - 2, (int)COLS - 12, "<health: %d>", player.Health);
}

void DrawBombs(Bomb bombs[])
{
    for (int b = 0; b < _MaximumBombs; b++)
    {
        if (bombs[b].Collision == true)
            continue;

        mvaddch(bombs[b].Position->Row, bombs[b].Position->Column, bombs[b].Symbol);
    }
}

bool IsGameOver(Player player, Invader invader[], int timeInSeconds)
{
    if (player.Health <= 0)
        return true;

    if (player.Position->Row == invader[(_InvaderPerRow * _InvaderRowCount) -1].Position->Row)
        return true;

    if (timeInSeconds == 0)
        return true;

    return false;
}

int ShowGameOverScreen(InputThread *inputThread, int score)
{
    int imageLength = 5;
    char ** image = (char **)malloc(sizeof(char *) * imageLength);

    for (int i = 0; i < imageLength; i++)
    {
        image[i] = malloc(sizeof(char) * 55 + 1 );
    }

    strcpy(image[0], "  ____    _    __  __ _____    _____     _______ ____  ");
    strcpy(image[1], " / ___|  / \\  |  \\/  | ____|  / _ \\ \\   / / ____|  _ \\ ");
    strcpy(image[2], "| |  _  / _ \\ | |\\/| |  _|   | | | \\ \\ / /|  _| | |_) |");
    strcpy(image[3], "| |_| |/ ___ \\| |  | | |___  | |_| |\\ V / | |___|  _ < ");
    strcpy(image[4], " \\____/_/   \\_\\_|  |_|_____|  \\___/  \\_/  |_____|_| \\_\\");

    int messageLength = 3;
    char ** message = (char **)malloc(sizeof(char *) * messageLength);

    for (int i = 0; i < messageLength; i++)
    {
        message[i] = malloc(sizeof(char) * 90 + 1 );
    }

    char *scoreString = (char*)malloc(90 * sizeof(char));
    // sprintf(scoreString, "your score: %d", score);
    sprintf(scoreString, "your score: %d", globalScore);
    strcpy(message[0], scoreString);
    strcpy(message[1], "");
    strcpy(message[2], "press any key to start .. or 'q' to quit");

    return PrintSplashScreen(inputThread ,image, imageLength, message, messageLength, false);
}

void ShowSplashScreen(InputThread *inputThread)
{
    int imageLength = 5;
    char ** image = (char **)malloc(sizeof(char *) * imageLength);

    for (int i = 0; i < imageLength; i++)
    {
        image[i] = malloc(sizeof(char) * 51 + 1 );
    }

    strcpy(image[0], "    _____   ___    _____    ____  __________  _____");
    strcpy(image[1], "   /  _/ | / / |  / /   |  / __ \\/ ____/ __ \\/ ___/");
    strcpy(image[2], "   / //  |/ /| | / / /| | / / / / __/ / /_/ /\\__ \\ ");
    strcpy(image[3], " _/ // /|  / | |/ / ___ |/ /_/ / /___/ _, _/___/ / ");
    strcpy(image[4], "/___/_/ |_/  |___/_/  |_/_____/_____/_/ |_|/____/  ");

    int messageLength = 1;
    char ** message = (char **)malloc(sizeof(char *) * messageLength);
    message[0] = malloc(sizeof(char) * 26 + 1 );
    strcpy(message[0], "press any key to start .. ");

    PrintSplashScreen(inputThread, image, imageLength, message, messageLength, false);
}

void GameLoop(InputThread *inputThread, int key, bool breakLoop, Player player, Invader invaders[], Projectile projectiles[], Bomb bombs[], Shield shields[])
{
    int dropBomb = 0;
    int timeInSeconds = _Time;
    while(true)
    {
        DrawPlayer(player);
        DrawBombs(bombs);
        DrawShields(shields);
        DrawProjectiles(projectiles);
        DrawInvaders(invaders); 
        DrawScore(player);
        DrawHealth(player);
        DrawTime(timeInSeconds);
        DrawTitle();
        refresh();

        key = inputThread->key;

        gameOver = IsGameOver(player, invaders, timeInSeconds);

        switch(key)
        {
            case KEY_ESC:
            case KEY_Q:
            case KEY_q:
                breakLoop = true;
                break;
            case KEY_A:
            case KEY_a:
            case KEY_LEFT:
                mvaddch(player.Position->Row, player.Position->Column, ' ');
                player.Position->Column--;
                releaseThreadKey(inputThread);
                break;
            case KEY_RIGHT:
            case KEY_D:
            case KEY_d:
                mvaddch(player.Position->Row, player.Position->Column, ' ');
                player.Position->Column++;
                releaseThreadKey(inputThread);
                break;
            case KEY_Space:
                if (gunCooldown <= 0)
                {
                    Shoot(projectiles, player);
                    gunCooldown = _FramesPerSecond; // 1 sec = _FramesPerSecond
                }
                releaseThreadKey(inputThread);
                break;
        } 
        

        /* 1000000 = 1s */
        usleep(1000000 / _FramesPerSecond);
        frameCounter++;
        if (frameCounter % _FramesPerSecond == 0
        && frameCounter > 0)
        {
            timeInSeconds--;
            frameCounter = 0;
        }
        if (gunCooldown > 0)
            gunCooldown = gunCooldown - 4;

        
        if((frameCounter % (_FramesPerSecond / 2) == 0)) //nach _FramesPerSecond Frames
        {
            MoveInvaders(invaders);
            gameOver = DetectCollision(&player, invaders, projectiles, bombs, shields, timeInSeconds);
            dropBomb++;
        }

        if((frameCounter % ((int)(_FramesPerSecond / 8)) == 0)) // 8x speed
        {
            MoveProjectiles(projectiles); 
            gameOver = DetectCollision(&player, invaders, projectiles, bombs, shields, timeInSeconds);        
            MoveBombs(bombs);   
            gameOver = DetectCollision(&player, invaders, projectiles, bombs, shields, timeInSeconds);          
        }

        if (dropBomb == 3)
        {
            DropBomb(invaders, bombs);
            dropBomb = 0;
            
        }
        
        if(breakLoop) break;
        if(gameOver) break;
    }
}

int RunGame(InputThread *inputThread)
{
    int key = 0;
    ShowSplashScreen(inputThread);
    globalScore = 0;
    
    /* setup vars */
    Player player;
    Invader invaders[_InvaderPerRow * _InvaderRowCount];
    Projectile projectiles[_MaximumProjectiles];
    Bomb bombs[_MaximumBombs];
    Shield shields[_MaximumShields]; 

    /* init player */
    Position * playerPosition = (Position *)malloc(sizeof(Position));;
    playerPosition->Column = COLS / 2;
    playerPosition->Row = LINES - 5;
    player.Health = 3;
    player.Score = 0;
    player.Symbol = _PlayerAppearence;
    player.Position = playerPosition;

    /* init invaders */
    int i = 0;
    while (i < (_InvaderPerRow * _InvaderRowCount))
    {
        Position * invaderPosition = (Position *)malloc(sizeof(Position));
        
        int row = i % _InvaderPerRow;
        int col = i / _InvaderPerRow;

        invaderPosition->Column = _InvaderFirstRow + row + ( row * _InvaderHorizontalSpace);
        invaderPosition->Row = _InvaderFirstColumn + col + (col * _InvaderVerticalSpace);
        invaders[i].Position = invaderPosition;

        invaders[i].Health = true;
        invaders[i].Direction = LEFT;
        invaders[i].SymbolOne = _InvaderAppearence;
        invaders[i].SymbolTwo = _InvaderAppearenceTwo;
        invaders[i].SymbolThree = _InvaderAppearenceThree;
        invaders[i].SymbolFour = _InvaderAppearenceFour;
        invaders[i].SymbolSwitch = ONE;
        i++;
    }

    /* init projectile array */
    i = 0;
    while (i < _MaximumProjectiles)
    {
        Position * projectilePosition = (Position *)malloc(sizeof(Position));

        projectiles[i].Position = projectilePosition;
        projectiles[i].Symbol = _DisabledAppearence;
        projectiles[i].Direction = UP;
        projectiles[i].Collision = true;
        i++;
    }

    /* init bomb array */
    i = 0;
    while (i < _MaximumBombs)
    {
        Position * bombPosition = (Position *)malloc(sizeof(Position));

        bombs[i].Position = bombPosition;
        bombs[i].Symbol = _DisabledAppearence;
        bombs[i].Direction = DOWN;
        bombs[i].Collision = true;
        i++;
    }

    /* init shields */
    i = 0;
    int cursor = 1;
    int ninth = (int) COLS / 9;
    /* set row for the shields */
    int shieldRow = (int) (LINES / 10) * 8;
    int firstShieldRow = shieldRow;

    while (i < _MaximumShields)
    {
        if (i == (_MaximumShields / 3) || i == (_MaximumShields / 3) * 2)
        {
            shieldRow++;
            cursor = 1;
        }

        Position * shieldPosition = (Position *)malloc(sizeof(Position));
        shieldPosition->Column = -1;
        shieldPosition->Row = -1;
        shields[i].Health = 5;
        if (cursor <= COLS)
        {
            while (cursor <= COLS)
            {
                if (firstShieldRow == shieldRow)
                {
                    if ((ninth + 1 < cursor && cursor <= ninth * 2 - 1)
                    || (ninth * 3 + 1 < cursor && cursor <= ninth * 4 - 1)
                    || (ninth * 5 + 1 < cursor && cursor <= ninth * 6 - 1)
                    || (ninth * 7 + 1 < cursor && cursor <= ninth * 8 - 1))
                    {
                        (*shieldPosition).Column = cursor;
                        (*shieldPosition).Row = shieldRow;
                        cursor++;
                        break;
                    }
                    if (cursor > _MaximumShields)
                    {
                        shields[i].Health = 0;
                        break;
                    }
                    cursor++;
                }
                else if (firstShieldRow + 2 == shieldRow)
                {
                    if ((ninth < cursor && cursor <= ninth + (ninth / 3))
                    || (ninth + ( (ninth / 3) * 2) < cursor && cursor <= ninth * 2)
                    || (ninth * 3 < cursor && cursor <= ninth * 3 + (ninth / 3))
                    || (ninth * 3 + ( (ninth / 3) * 2) < cursor && cursor <= ninth * 4)
                    || (ninth * 5 < cursor && cursor <= ninth * 5 + (ninth / 3))
                    || (ninth * 5 + ( (ninth / 3) * 2) < cursor && cursor <= ninth * 6)
                    || (ninth * 7 < cursor && cursor <= ninth * 7 + (ninth / 3))
                    || (ninth * 7 + ( (ninth / 3) * 2) < cursor && cursor <= ninth * 8))
                    {
                        (*shieldPosition).Column = cursor;
                        (*shieldPosition).Row = shieldRow;
                        cursor++;
                        break;
                    }
                    if (cursor > _MaximumShields)
                    {
                        shields[i].Health = 0;
                        break;
                    }
                    cursor++;
                }
                else
                {
                    if ((ninth < cursor && cursor <= ninth * 2)
                    || (ninth * 3 < cursor && cursor <= ninth * 4)
                    || (ninth * 5 < cursor && cursor <= ninth * 6)
                    || (ninth * 7 < cursor && cursor <= ninth * 8))
                    {
                        (*shieldPosition).Column = cursor;
                        (*shieldPosition).Row = shieldRow;
                        cursor++;
                        break;
                    }
                    if (cursor > _MaximumShields)
                    {
                        shields[i].Health = 0;
                        break;
                    }
                    cursor++;
                }
            }
        }
        else
        {
            shields[i].Health = 0;
        }
        shields[i].Position = shieldPosition;
        shields[i].SymbolOne = _ShieldAppearence;
        shields[i].SymbolTwo = _ShieldAppearenceTwo;
        shields[i].SymbolThree = _ShieldAppearenceThree;
        shields[i].SymbolFour = _ShieldAppearenceFour;
        shields[i].SymbolSwitch = ONE;
        i++;
    }

    bool breakLoop = false;
    /* ================================================================================================================= */
    /* =================================================== GAME LOOP =================================================== */
    /* ================================================================================================================= */
    GameLoop(inputThread, key, breakLoop, player, invaders, projectiles, bombs, shields);

    int score = globalScore;
    bool allDefeated = true;

    for(int i = 0; i < _InvaderPerRow * _InvaderRowCount; i++)
    {
        if (invaders[i].Health)
            allDefeated = false;
    }

    if (allDefeated && player.Health != 0)
        score = score * player.Health;

    Dispose(player, invaders, projectiles, bombs, shields);

    if (allDefeated)
        return ShowWonScreen(inputThread, score);

    return ShowGameOverScreen(inputThread, score);
}

int PrintSplashScreen(InputThread *inputThread, char ** image, int imageLength, char ** message, int messageLength, bool won)
{
    ClearTerminal();
    
    int fifth = (int) LINES / 5;
    int startRow = fifth * 2;
    int startColumn = ((int) COLS / 2) - 25;

    if (won)
    {
        startRow = 0;
    }

    for (int i = 0; i < imageLength; i++)
    {
        mvaddstr(startRow + i, startColumn, image[i]);
    }

    for (int m = 0; m < messageLength; m++)
    {
        if (won)
            mvaddstr(imageLength + 3, startColumn, message[m]);
        else
            mvaddstr(fifth * 4 + m, startColumn + 1, message[m]);
    }

    refresh();
    releaseThreadKey(inputThread);
    int result = 0;
    
    while (true)
    {
        result = inputThread->key;
        if (result != 0)
            break;

        usleep(500);
    }
    releaseThreadKey(inputThread);

    for(int i = messageLength - 1; i >= 0; i--)
    {
        free(message[i]);
    }
    free(message);
    for(int i = imageLength - 1; i >= 0; i--)
    {
        free(image[i]);
    }
    free(image);
    /* clear screen */
    ClearTerminal();
    refresh();

    /* quit game on these keys */
    if (result == KEY_Q || result == KEY_q || result == KEY_ESC)
        return 1;
    else
        return 0;
}

void DropBomb(Invader invader[], Bomb bombs[])
{
    for (int i = 0; i < _InvaderPerRow * _InvaderRowCount; i++)
    {
        if (invader[i].Health == false)
            continue;

        int random = rand();      // Returns a pseudo-random integer between 0 and RAND_MAX.
        if (random < RAND_MAX / 4) // 1/4 = 25% drop chance
        {
            for (int b = 0; b < _MaximumBombs; b++)
            {
                if (bombs[b].Collision == true)
                {
                    bombs[b].Symbol = _BombAppearence;
                    bombs[b].Direction = DOWN;
                    bombs[b].Position->Column = invader[i].Position->Column;
                    bombs[b].Position->Row = invader[i].Position->Row + 1;
                    bombs[b].Collision = false;
                    
                    break;
                }

            }
        }
    }

}

void MoveBombs(Bomb bombs[])
{
    for(int b = 0; b < _MaximumBombs; b++)
    {
        if(bombs[b].Collision == false) // solange projektile keine Collision haben, flieg
        {
            DeleteChar(bombs[b].Position);
            bombs[b].Position->Row++;     

            if (bombs[b].Position->Row > LINES - 5)
                bombs[b].Collision = true;
        }
    }
}

void DrawTitle()
{
    mvprintw(0, ((int)COLS / 2) - 18, "..:: SPACE INVADERS TIME ATTACK ::..");
}

int ShowWonScreen(InputThread *inputThread, int score)
{
    int imageLength = 25;
    char ** image = (char **)malloc(sizeof(char *) * imageLength);

    for (int i = 0; i < imageLength; i++)
    {
        image[i] = malloc(sizeof(char) * 52 + 1 );
    }

    strcpy(image[0], "MMMMMMMMMMWNNNWMMMMMMMMMMMMMMMMMMMWNXXXNMMMMMMMMMM");
    strcpy(image[1], "MMMMMMMMMWx,',dNMMMMMMMMMMMMMMMMMMXl...oNMMMMMMMMM");
    strcpy(image[2], "MMMMMMMMMNl. .:KNNWMMMMMMMMMMMMWNN0;   cNMMMMMMMMM");
    strcpy(image[3], "MMMMMMMMMWX00Oo'.'dNMMMMMMMMMMNo..'oOOOXWMMMMMMMMM");
    strcpy(image[4], "MMMMMMMMMMNXXXl   ;0XXXXXXXXXX0,   oXNNWMMMMMMMMMM");
    strcpy(image[5], "MMMMMMMMMNo....    ............    ...'dNMMMMMMMMM");
    strcpy(image[6], "MMMMMMWWWX:                            :KNNWMMMMMM");
    strcpy(image[7], "MMMMMWx,,'.   ;xkx,            :kkd'   ..''dWMMMMM");
    strcpy(image[8], "MMWWNXc       lNWXc            oNWK;       :KXXNMM");
    strcpy(image[9], "MWd,''.       .','.            .,,'.        ...dWM");
    strcpy(image[10], "MNc                                            cNM");
    strcpy(image[11], "MNc   .odo.                            .ldl.   cNM");
    strcpy(image[12], "MNc   :XMX:                            ;XMX;   cNM");
    strcpy(image[13], "MNc   :XMX:   'oddddddddddddddddddl.   ;XMX;   cNM");
    strcpy(image[14], "MNc   :NMX:   :XWWWWWWWMMMMWWWWWWW0,   :XMX:   cNM");
    strcpy(image[15], "MWKxxx0WMW0xxd:,,,,,,,oNMMNd,,,,,,':dxx0WMW0xdx0WM");
    strcpy(image[16], "MMMMMMMMMMMMMNc       ;XMMN:       lWMMMMMMMMMMMMM");
    strcpy(image[17], "MMMMMMMMMMMMMW0oooooooONMMWOooooood0WMMMMMMMMMMMMM");
    strcpy(image[18], "MMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMMM");
    strcpy(image[19], "");
    strcpy(image[20], " ____  _   _ ______     _______     _______ ____  _ ");
    strcpy(image[21], "/ ___|| | | |  _ \\ \\   / /_ _\\ \\   / / ____|  _ \\| |");
    strcpy(image[22], "\\___ \\| | | | |_) \\ \\ / / | | \\ \\ / /|  _| | | | | |");
    strcpy(image[23], " ___) | |_| |  _ < \\ V /  | |  \\ V / | |___| |_| |_|");
    strcpy(image[24], "|____/ \\___/|_| \\_\\ \\_/  |___|  \\_/  |_____|____/(_)");

    int messageLength = 3;
    char ** message = (char **)malloc(sizeof(char *) * messageLength);

    for (int i = 0; i < messageLength; i++)
    {
        message[i] = malloc(sizeof(char) * 90 + 1 );
    }

    char *scoreString = (char*)malloc(99 * sizeof(char));
    sprintf(scoreString, "your winner score: %d", score);
    strcpy(message[0], scoreString);
    strcpy(message[1], "");
    strcpy(message[2], "press any key to start .. or 'q' to quit");

    return PrintSplashScreen(inputThread ,image, imageLength, message, messageLength, true);
}
