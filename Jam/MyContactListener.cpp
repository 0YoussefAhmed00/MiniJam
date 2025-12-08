#include "MyContactListener.h"
#include <iostream>

void MyContactListener::BeginContact(b2Contact* contact)
{
    std::cout << "Contact Begin\n";
}

void MyContactListener::EndContact(b2Contact* contact)
{
    std::cout << "Contact End\n";
}
