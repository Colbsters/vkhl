#include <vkhl/vkhl.hpp>

#include <iostream>

int main()
{
	vkhl::Defer deferedAction([](){
			std::cout << "Defered Hello World\n";
		});

	std::cout << "Hello World\n";

	return 0;
}
