#include <random>
#include <array>
#include <iostream>
#include <list>
#include <algorithm>
#include <vector>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <list>
#include <deque>
#include <fstream>
#include <string>



// Import things we need from the standard library
using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::this_thread::sleep_for;
using std::array;
using std::list;
using std::cout;
using std::endl;
using std::find;
using std::random_device;
using std::mt19937;
using std::uniform_int_distribution;

#define both 20
#define HEIGHT both
 #define WIDTH both
#define percentage_walls 10
#define WEIGHT 1

random_device rdi; //takes system information and uses to create a truely random number (use as seed for mt)
mt19937 mti(rdi()); //Mersenne Twister pseudo-random generator of 32-bit numbers with a state size of 19937 bits

int random_in_range(int min, int max)
{
	uniform_int_distribution<int> dist(min, max); //between which range should numbers be generated
	return dist(mti); //return a number of the set range
}

struct Node {
	double g = 10,
		   f = 0;

	int	xPos = 0,
		yPos = 0,
		value = (random_in_range(1, 100) <= percentage_walls) ? -2 : -1;;
		//value = -1; //-2 for wall, -1 default, 0 start, -3 end;

	bool closed = 0,
		 open = 0;

	Node * parent = nullptr;
};
// Define the alias "the_clock" for the clock type we're going to use.
typedef std::chrono::steady_clock the_clock;

//Make these easier to type
typedef array<array<Node, WIDTH>, HEIGHT> grid_type;

void printGrid(grid_type & grid_to_print) {
	for (int y = HEIGHT - 1; y >= 0; y--) {
		for (int x = 0; x < WIDTH; x++) {
			if (grid_to_print[y][x].value > -1 && grid_to_print[y][x].value < 10) {
				cout << " ";
			}
			if (!grid_to_print[y][x].closed) {
				cout << grid_to_print[y][x].value << " ";
			} else{
				cout << " C" << " ";
			}
		}
		cout << endl;
	}
}

void printPath(std::list<Node> & list_to_print, Node & start, Node & end) {
	cout << "pathing from x:" << start.xPos << " y:" << start.yPos << endl;
	for (auto it : list_to_print) {
		std::cout << "x: " << it.xPos << " y: " << it.yPos << std::endl;
	}
	cout << "...to x:" << end.xPos << " y:" << end.yPos << endl;

}


Node* node_with_lowest_f_(list<Node*> node_list, Node & end) {
	auto lowest_f_node = *node_list.begin();
	for (auto node : node_list) {
		//if node is < current lowest then make current lowest = node
		lowest_f_node = (node->f < lowest_f_node->f) ? node : lowest_f_node;
		if (lowest_f_node == &end) {
			return lowest_f_node;
		}
	}
	return lowest_f_node;
}

//in list( node, list)
bool in_list_(Node * node, list<Node*> node_list) {
	return (find(node_list.begin(), node_list.end(), node) != node_list.end());
}

//check if coords are viable
bool on_grid_(int y, int x) {
	return (x >= 0 && y >= 0 && x < WIDTH && y < HEIGHT);
}

//Euclidean
double heuristic__(Node & node, Node & end) {
	auto dx = std::abs(node.xPos - end.xPos);
	auto dy = std::abs(node.yPos - end.yPos);

	//return D * sqrt(dx * dx + dy * dy)
	return (10*14.142136)*(sqrt((dx*dy) + (dy*dy)));

	//old alt (apparently terrible)
	//return WEIGHT*hypot(pow(node.xPos - end.xPos, 2), pow(node.yPos - end.yPos, 2));
}

//diagonal
double heuristic_(Node & node, Node & end) {
	auto dx = std::abs(node.xPos - end.xPos);
	auto dy = std::abs(node.yPos - end.yPos);

	//return D * (dx + dy) + (D2 - 2 * D) * min(dx, dy)
	return (14 * 14.142136)*(dx + dy) + (14.142136 - 2 * 10) * std::min(dx, dy);
}

//manhattan
double heuristic(Node & start, Node & end) {
	return (std::abs(start.xPos - end.xPos) + std::abs(start.yPos - end.yPos));
}

bool valid_movement(Node & current_position, int attempted_y, int attempted_x, grid_type & grid) {
	//if moving diagonally
	if (current_position.yPos - attempted_y != 0 && current_position.xPos - attempted_x != 0) {
		//if you try to clip a corner
		if (grid[current_position.yPos][attempted_x].value == -2 || grid[attempted_y][current_position.xPos].value == -2) {
			//invalid movement
			return false;
		}
	}
	
	//valid movement
	return true;
}

Node check_neighbours(Node & current_coord, grid_type & grid, Node & start, Node & end) {
	//init neighbour as last step point
	auto neighbour = current_coord;
	
	//initially prioritize non diagonal movements
	for (int switchToDiagonal = 0; switchToDiagonal < 2; switchToDiagonal++) {
		//test all combinations of neighgbouring nodes
		for (int y = -1; y < 2; y++) {
			for (int x = -1; x < 2; x++) {
				if (y == 0 && x == 0) {
					//if not a neighbour skip
					continue;
				}

				if (!switchToDiagonal) {
					if (y != 0 && x != 0) {
						continue;
					}
				}

				if (on_grid_(current_coord.yPos + y, current_coord.xPos + x)) {
					//if this neighbour is a possible step for path
					if (grid[current_coord.yPos][current_coord.xPos].value - 1 == grid[current_coord.yPos + (y)][current_coord.xPos + (x)].value) {
						//lastly, is this valid movement
						if (valid_movement(current_coord, current_coord.yPos + (y), current_coord.xPos + (x), grid)) {
							neighbour.yPos = current_coord.yPos + (y);
							neighbour.xPos = current_coord.xPos + (x);
							return neighbour;
						}
					}
				}
			}
		}
	}

	return neighbour;
}

void changeCellVal(int y, int x, int & modifier, grid_type & grid, Node & current, bool & end_flag) {
	if (on_grid_(y, x) && valid_movement(current, y, x, grid)) {
		(grid[y][x].value == -1 || grid[y][x].value == -3) ? end_flag = false, grid[y][x].value = modifier + 1 : false;
	}	
}

list<Node> traceBack(Node & start, Node & end, grid_type & grid) {
	//loop until endpoint changes value
	std::list<Node> path;

	Node nextStep;
	nextStep = end;
	Node lastStep;
	//add end point as last step in the path
	path.push_front(nextStep);

	//as long as next step's xy is not the start point
	for (int i = grid[end.yPos][end.xPos].value; i != 0; i--) {
		//if current position = 1 less than previous value
		//not currently updating area of search so this will select any rather than just neighbouring
		lastStep = nextStep;
		nextStep = check_neighbours(lastStep, grid, start, end);
		if (nextStep.xPos != lastStep.xPos || nextStep.yPos != lastStep.yPos) {//add neighbour check pls
			path.push_front(nextStep);
			//cout << nextStep.xPos << "," << nextStep.yPos << endl;
		}
	}
	return path;
}

bool possiblePaths(Node & start, Node & end, grid_type & grid) {
	//loop until endpoint changes value
	for (int loop = 0; grid[end.yPos][end.xPos].value == -3; loop++) {
		bool end_flag = true;
		for (int y = 0; y != HEIGHT; y++) {
			for (int x = 0; x != WIDTH; x++) {
				//for neighbours
				for (int y_modifier = -1; y_modifier < 2; y_modifier++) {
					for (int x_modifier = -1; x_modifier < 2; x_modifier++) {
						if (x_modifier == 0 && y_modifier == 0){
							//not a neighbour, skip
							continue;
						}
						if (!on_grid_(y_modifier + y, x_modifier + x)) {
							continue;
						}
						//if current value is e.g. 2 surrounding positions become 3
						if (grid[y][x].value == loop) {
							changeCellVal(y_modifier + y, x_modifier + x, loop, grid, grid[y][x], end_flag);
						}
					}
				}
			}
		}
		printGrid(grid);
		std::cin.ignore();
		system("CLS");

		if (end_flag) {
			return 0;
		}
	}
	return 1;
}

list<Node> astar_(Node & start, Node & end, grid_type & grid, bool & isPath) {

	//compute f/g/h for start node
	start.g = 0;
	start.f = start.g + heuristic_(start, end);

	list<Node*> open,		// the nodes to be evaluated
				closed;		// the nodes which have already been evaluated

	list<Node> path;		// the final path

	double distance_between;	//instead of calculating (14 for diagonal, 10 default)

	Node * node = nullptr;	//for handling neighbour, and later path
										
	open.push_front(&start);//put start node in the open list

	while (!open.empty()) {
		printGrid(grid);
		std::cin.ignore();
		system("CLS");
		
		
		//pick the most promising node to look at next.
		auto current = node_with_lowest_f_(open, end);

		if (current == &end) {
			//found the path - finished!
			break;
		}

		open.remove(current);
		current->open = false;

		closed.push_front(current);
		current->closed = true;

		//for each neighbour of current node
		//for possible neighbours
		for (int x_modifier = -1; x_modifier < 2; x_modifier++) {
			for (int y_modifier = -1; y_modifier < 2; y_modifier++) {

				//if not a neighbour or not on grid
				if (x_modifier == 0 && y_modifier == 0 || !on_grid_(current->yPos + y_modifier, current->xPos + x_modifier)) {
					continue;
				}

				node = &grid[current->yPos + y_modifier][current->xPos + x_modifier];

				//already finished with this node
				if (node->closed) {
					continue;
				}

				if (node->value == -2) {
					continue;
				}

				if (x_modifier != 0 && y_modifier != 0) {
					//moving diagonally
					distance_between = 14.142136;

					//if you try to clip a corner
					if (grid[current->yPos + y_modifier][current->xPos].value == -2 || grid[current->yPos][current->xPos + x_modifier].value == -2) {
						continue;
					}
				}
				else {
					//moving horizontally
					distance_between = 10;
				}
				//if moving horizontally distance between = 10, if moving diagonally distance between = 14
				//(x_modifier != 0 && y_modifier != 0) ? distance_between = 10 : distance_between = 14;

				auto new_g = current->g + distance_between;

				//not seen this node before
				if (!node->open) { //!in_list_(node, open)
					open.push_front(node);
				}
				else if (new_g >= node->g) {
					//seen this node before, but already found a shorter path to it
					continue;
				}

				node->g = new_g;
				node->f = node->g + heuristic_(*node, end);
				node->parent = current;
			}
		}
	}
	//Back Trace
	node = &end;
	while (node->parent) {
		path.push_front(*node);
		node = node->parent;
	}
	(node != &start) ? isPath = false : path.push_front(start);

	return path;
}

int main() {

	std::ofstream Path_Times;
	Path_Times.open("Path_Times.csv", std::ios_base::app);
	Path_Times << endl << "Times for " << WIDTH << "x" << HEIGHT << " in nanoseconds" << endl;
	Path_Times << "A-Star" << ", " << "Lee" << endl;

	std::cout << "Generating Grids, Finding paths and saving results" << std::endl;

	for (int loops = 1; loops <= 1; ++loops) {
		cout << "Iteration: " << loops << endl;
		grid_type *grid = new grid_type;
		//array of array of nodes

		//initialize grid
		for (int y = 0; y < HEIGHT; y++) {
			for (int x = 0; x < WIDTH; x++) {
				(*grid)[y][x].xPos = x;
				(*grid)[y][x].yPos = y;
			}
		}

		//start point
		auto startX = 0;
		auto startY = 0;
		(*grid)[startY][startX].value = 0;

		//end point
		auto endX = WIDTH - 1;
		auto endY = HEIGHT - 1;
		(*grid)[endY][endX].value = -3;

		//is there a path
		bool isPath = true;

		//Start time for A star
		auto start_time_astar = std::chrono::high_resolution_clock::now();

		//A Star Algorithm
		astar_((*grid)[startY][startX], (*grid)[endY][endX], (*grid), isPath);

		//Stop timing astar
		auto end_time_astar = std::chrono::high_resolution_clock::now();

		//print out astar result if there was a path
		auto time_taken_astar = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_astar - start_time_astar).count();

		////Start timing lee
		auto start_time_lee = std::chrono::high_resolution_clock::now();

			isPath = possiblePaths((*grid)[startY][startX], (*grid)[endY][endX], (*grid));
			if (isPath) {
				traceBack((*grid)[startY][startX], (*grid)[endY][endX], (*grid));
			}
			else {
				isPath = ((*grid)[startY][startX].xPos == (*grid)[endY][endX].xPos && (*grid)[startY][startX].yPos == (*grid)[endY][endX].yPos);
			}	


		////Stop timing
		auto end_time_lee = std::chrono::high_resolution_clock::now();

		////print out astar result if there was a path
		auto time_taken_lee = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time_lee - start_time_lee).count();

		if (isPath) {
			Path_Times << time_taken_astar << ", " << time_taken_lee << endl;
		}

		delete grid;	

	}
	cout << "Done!" << endl;
	Path_Times.close();
	return 0;
}



//try and implement priority queue.
