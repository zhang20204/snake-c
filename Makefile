all:app

app: snake.c
	gcc $^ -o $@
