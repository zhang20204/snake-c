#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/ioctl.h>

// 蛇的移动方向
#define UP 1
#define DOWN 2
#define LEFT 3 
#define RIGHT 4

//每一节蛇体的二维坐标
struct coordinate_2D {
	int x;
	int y;
};
// 蛇 结构体
struct snake {
	int speed_us; //微秒级别
	int length;
	struct coordinate_2D body[100];
};

//以非阻塞的方式获取输入的字符
int kbhit(char* ret) {
	struct termios oldt, newt;
	int ch;
	int oldf;

	//非阻塞 获取标准输入
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);

	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

	ch = getchar();

	//恢复阻塞状态
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);

	//判断是否有输入
	if (ch != EOF) {
		*ret = ch;
		return 1;
	}
	return 0;
}

//移动光标位置
void gotoxy(int x,int y)
{
	printf("%c[%d;%df",0x1B,y,x);
}

//生成一条蛇，其头的位置为(head_x, head_y), 长度为lenght
struct snake gen_snake(int head_x, int head_y, int lenght) {
	struct snake s;

	for(int i = 0; i<lenght; i++) {
		s.body[i] = (struct coordinate_2D){head_x, head_y+i};
	}
	s.length = lenght;
	s.speed_us = 500000;//500 ms
	return s;
}
void snake_move(struct snake s) {
	//蛇头
	gotoxy(s.body[0].x, s.body[0].y);
	printf("+\n");
	for(int i = 1; i < s.length; i++) {
		gotoxy(s.body[i].x, s.body[i].y);
		printf("o\n");
	}
	//光标回到左顶点
	gotoxy(10, 10);
}

//确定移动方向
void try_get_direction(int* direction) {
	//默认向上移动
	static int ret = UP;
	char getc;

	if (kbhit(&getc)) {
		switch(getc) {
			case 'k':
				ret = UP;
				break;
			case 'j':
				ret = DOWN;
				break;
			case 'h':
				ret = LEFT;
				break;
			case 'l':
				ret = RIGHT;
				break;
			default:
				break;
		}
	}
	*direction = ret;
	return;
}
//按指定方向移动蛇的位置
void updata_snake_info(struct snake* s, int direction) 
{
	struct coordinate_2D snake_tail = s->body[s->length-1];
	for(int i = s->length - 1; i>0; i--) {
		s->body[i] = s->body[i-1];
	}
	switch(direction) {
		case UP:
			s->body[0] = (struct coordinate_2D){s->body[0].x, s->body[0].y - 1};
			break;
		case DOWN:
			s->body[0] = (struct coordinate_2D){s->body[0].x, s->body[0].y + 1};
			break;
		case LEFT:
			s->body[0] = (struct coordinate_2D){s->body[0].x - 1 , s->body[0].y};
			break;
		case RIGHT:
			s->body[0] = (struct coordinate_2D){s->body[0].x + 1 , s->body[0].y};
			break;
		default:
			break;
	}

	gotoxy(snake_tail.x, snake_tail.y);
	printf(" ");

	//光标回到左顶点
	gotoxy(1, 1);

}

//从 (left_top_x, left_top_y)开始画宽高分别为consoleWidth, consoleHeight的边框
void load_env(struct coordinate_2D left_top, struct coordinate_2D wall)
{
	char WALL = '=';
	int x, y;

	for (y = left_top.y; y < left_top.y+wall.y; y++)
	{
		gotoxy(left_top.x, y); //Left Wall
		printf("%c\n",WALL);

		gotoxy(left_top.x + wall.x, y); //Right Wall
		printf("%c\n",WALL);
	}
	for(x = left_top.x; x < left_top.x + wall.x; x++) {
		gotoxy(x, left_top.y);		//top wall
		printf("%c\n", WALL);

		gotoxy(x, left_top.y + wall.y);//down wall
		printf("%c\n", WALL);
	}

	return;
}
void load_food(struct coordinate_2D food) {
	gotoxy(food.x, food.y);
	printf("@\n");
}

bool is_safety(struct snake s, struct coordinate_2D wall_left_top, struct coordinate_2D wall) {
	bool flag = true;

	if (s.body[0].x == wall_left_top.x) { //left
		flag = false;
	}
	if (s.body[0].x == wall_left_top.x + wall.x) { //right
		flag = false;
	}
	if (s.body[0].y == wall_left_top.y) {//top
		flag = false;
	}
	if (s.body[0].y == wall_left_top.y + wall.y) {//down
		flag = false;
	}
	return flag;
}

bool try_eat_food(struct snake* s, struct coordinate_2D* food) {
	if (s->body[0].x == food->x && s->body[0].y == food->y) {
		//蛇体增加1个单位
		s->body[s->length] = s->body[s->length-1];
		s->length++;
		//移动速度提升1个单位
		s->speed_us -= 1000; //速度提升1豪秒

		return true;
	}

	return false;
}

//新生食物位置改变
void updata_food_info(struct coordinate_2D* food, struct coordinate_2D wall_left_top, struct coordinate_2D wall) {
	srand((unsigned)time(0));
	 
	int x = rand()%(wall.x-1) + 1 + wall_left_top.x;
	int y = rand()%(wall.y-1) + 1 + wall_left_top.y;
	*food = (struct coordinate_2D){x, y}; 
}


void snake_speed(struct snake* snake) {
	usleep(snake->speed_us);
}

void get_console_attr(int* w, int* h) {
	struct winsize ws;

	ioctl(STDIN_FILENO, TIOCGWINSZ, &ws);
	*w = ws.ws_col;
	*h = ws.ws_row;
}

int main(int argc, char* argv[])
{
	struct coordinate_2D console_attr;
	int direction; //移动方向
	struct coordinate_2D wall_left_top; //墙体的做上顶点
	struct coordinate_2D wall; //墙体的宽和高
	struct coordinate_2D food;//食物位置
	struct snake s; //蛇

	get_console_attr(&console_attr.x, &console_attr.y);

	wall_left_top = (struct coordinate_2D){0, 0}; //全屏情况加可以选择游戏区域的位置(前提是在设置wall时不要设置太大)
	wall.x = console_attr.x;
	wall.y = console_attr.y-3; //留出3行显示其他信息

	food = (struct coordinate_2D){wall_left_top.x+10, wall_left_top.y+10};

	s = gen_snake(wall_left_top.x + wall.x/2, wall_left_top.y + wall.y/2, 3);

	//清屏幕
	system("clear");
	//绘制围墙
	load_env(wall_left_top, wall);

	//主循环
	for(;;) {
		//绘制食物
		load_food(food);

		//尝试根据输入调整运动方向
		try_get_direction(&direction);
		updata_snake_info(&s, direction);

		//判断是否出界
		if(!is_safety(s, wall_left_top, wall)) {
			gotoxy(wall_left_top.x, wall_left_top.y + wall.y + 3);
			printf("game over\n");
			return 1;
		}

		//绘制snake
		snake_move(s);

		//关于吃到食物的处理过程
		if (try_eat_food(&s, &food)) {
			updata_food_info(&food, wall_left_top, wall);
		}

		//其他信息
		gotoxy(wall_left_top.x, wall_left_top.y + wall.y + 1);
		printf("time of moved:%d us\n", s.speed_us);
		gotoxy(wall_left_top.x, wall_left_top.y + wall.y + 2);
		printf("snake's lenght = %d\n", s.length);

		snake_speed(&s); //每次循环等待的时间来衡量
	}

	return 0;
}
