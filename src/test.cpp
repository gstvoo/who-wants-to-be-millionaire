#include <SFML/Graphics.hpp>
#include <string>

int main()
{
    // Create the window
    sf::RenderWindow window(sf::VideoMode(250, 150), "SFML window");

    // Create the font and label
    sf::Font font;
    if (!font.loadFromFile("arial.ttf")) {
        return EXIT_FAILURE;
    }
    sf::Text label("Count: 0", font, 20);
    label.setPosition(100, 50);

    // Create the button
    sf::RectangleShape button(sf::Vector2f(100, 30));
    button.setPosition(75, 90);
    button.setFillColor(sf::Color::Green);

    // Create the count variable
    int count = 0;

    while (window.isOpen())
    {
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::MouseButtonPressed && event.mouseButton.button == sf::Mouse::Left) {
                sf::Vector2f mousePos = window.mapPixelToCoords(sf::Vector2i(event.mouseButton.x, event.mouseButton.y));
                if (button.getGlobalBounds().contains(mousePos)) {
                    count++;
                    label.setString("Count: " + std::to_string(count));
                }
            }
        }

        window.clear(sf::Color::White);
        window.draw(label);
        window.draw(button);
        window.display();
    }

    return EXIT_SUCCESS;
}
