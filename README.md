# Grandmaster Connect Four AI

A hyper-optimized, mathematically perfect Connect Four engine built with a bare-metal C++ backend, wrapped in a stateless Python/Flask API, and served via a modern, animated web interface. 

This project demonstrates a full-stack microservice architecture, successfully separating a computationally heavy algorithm from a lightweight browser frontend.

## 🧠 The Architecture

This application is decoupled into three distinct layers:

1. **The Core Engine (C++)**: A highly concurrent, multi-threaded C++ executable. It uses a 64-bit Bitboard representation to evaluate board states at roughly 5+ Million Nodes Per Second. The engine utilizes a Negamax algorithm with Alpha-Beta pruning, protected from combinatorial explosion by a 1 GB lockless Transposition Table.
2. **The API Bridge (Python/Flask)**: A local web server acting as the translation layer. It receives HTTP `POST` requests from the frontend, securely executes the C++ binary in a stateless `--api` mode, parses standard output, and returns the mathematically perfect move as a JSON payload.
3. **The Frontend UI (HTML/CSS/JS)**: A responsive, zero-dependency interface. It features CSS grid layouts, dynamic dropping physics, animated gradient backgrounds, and browser-based `localStorage` state management to track lifetime win/loss records.

## ✨ Key Features

* **Perfect Opening Book**: The engine utilizes a pre-calculated, mathematically flawless 8-ply opening dictionary. It instantly matches against 129,498 canonical board states before transitioning to live heuristic searches.
* **Deep Mid-Game Search**: Capable of searching 24+ plies deep into the game tree to find forced wins or trap the opponent.
* **Stateless API Design**: The C++ engine wakes up, calculates a single perfect response based on the move history, prints the result, and shuts down instantly—preventing memory leaks and allowing for infinitely scalable web requests.
* **Persistent Game State**: The UI automatically saves your active game and lifetime scoreboard to the browser, allowing you to refresh the page without losing your match.

## 🚀 How to Run Locally

Follow these steps to compile the engine and spin up the web application on your local machine.

### Prerequisites
* A C++ compiler (e.g., `g++`, MinGW, or MSVC)
* Python 3.x installed
* ~1 GB of available RAM (to fully map the 64-bit Transposition Table without hitting the SSD swap file)

### Step 1: Clone the Repository
Download this project to your local machine and navigate into the folder.

`git clone [https://github.com/YOUR_USERNAME/ConnectFourAI.git](https://github.com/YOUR_USERNAME/ConnectFourAI.git)`

`cd ConnectFourAI`

Step 2: Compile the C++ Engine

Compile the source code into an executable named engine.exe (or ./engine on Linux/Mac). Ensure your compiler flags are set for maximum speed optimization (e.g., -O3).

`g++ -O3 main.cpp connectfour.cpp -o engine.exe`

Step 3: Install Python Dependencies

The Python bridge requires Flask and CORS to communicate with the browser. Install them via pip:

`pip install flask flask-cors`

Step 4: Start the API Server

Run the Python script to spin up the local server. It will listen for incoming moves on Port 5000.

`python gui.py`

Step 5: Enter the Arena

Leave the terminal running in the background. Open your file explorer and simply double-click the index.html file to open it in Chrome, Edge, or Firefox.

The API will automatically initialize the engine, the AI will take the first move (Yellow) in the center column, and the game will begin.

🛠️ Tech Stack

    `Backend Computations: C++17`

    API Framework: Python 3, Flask

    Frontend: Vanilla HTML5, CSS3, JavaScript

    Celebration Effects: canvas-confetti CDN
