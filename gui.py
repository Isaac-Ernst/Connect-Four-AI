
from flask import Flask, request, jsonify
from flask_cors import CORS
import subprocess

app = Flask(__name__)
CORS(app) 

@app.route('/get_move', methods=['POST'])
def get_move():
    data = request.json
    move_history = data.get('history', '')
    
    try:
        result = subprocess.run(
            ['./engine.exe', '--api', move_history], 
            capture_output=True, 
            text=True,
            check=True
        )
        
        # THE FIX: Split the C++ output into a list of words, 
        # and strictly grab the very last word (which will be the AI's move)
        raw_output = result.stdout.strip().split()
        ai_move = int(raw_output[-1]) 
        
        return jsonify({'ai_move': ai_move})
        
    except Exception as e:
        # If it crashes again, this will print the exact C++ error to your browser console
        error_details = result.stdout if 'result' in locals() else str(e)
        return jsonify({'error': error_details}), 500
        # file:///C:/Users/6stri/ConnectFour/index.html

if __name__ == '__main__':
    print("🚀 Connect Four API is running on http://localhost:5000")
    app.run(port=5000)
