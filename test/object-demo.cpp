/*
 * object_demo.cpp
 *
 * Author: Lain Musgrove (lain.proliant@gmail.com)
 * Date: Sunday May 31, 2020
 *
 * Distributed under terms of the MIT license.
 */

#include <iostream>
#include "jotdown/object.h"

using namespace jotdown::object;

int main() {
    Document doc;

    auto& section = doc.add(new Section(1));
    auto& text_block = section.add(new TextBlock());
    text_block.add(new Text("This is the best yay!"));

    section.header().add(new Text("Whee!"));

    std::cout << doc.to_json().to_string(true) << std::endl;
    std::cout << std::endl;
    std::cout << text_block.to_json().to_string(true) << std::endl;
}
