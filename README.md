# Gangtic-Phone: Drawing Quiz Game using TCP Cocket

<br>

### 👋 Introduction

<table>
    <tr>
        <th colspan="2">Project Name</th>
        <th colspan="4">Gangtic-Phone</th>
    </tr>
    <tr>
        <th colspan="2">Period</th>
        <th colspan="4">2024.10 ~ 2024.12</th>
    </tr>
    <tr>
        <th colspan="2">Team Member</th>
        <th colspan="4"><a href="https://github.com/kangeunsong">Eunsong Kang</a> (BACK-END) <br><a href="https://github.com/gaeunYoo23">Gaeun Yoo</a> (FRONT-END)</th>
    </tr>
      <tr>
        <th>OS</th>
        <th>Linux (Ubuntu)</th>
        <th>Language</th>
        <th>C</th>
        <th>OSS</th>
        <th>SDL2 Library</th>
    </tr>
</table>

<br>
<br>
<br>
<br>

### 🎨 소개

This is the term-project for **2024-2 Server Programming of CBNU**.

<br>

**Gangtic-Phone** is a drawing quiz game built using a **_server-client architecture_** and implemented with the **_SDL library_** for a GUI-based experience.

<br>

Inspired by the popular game Gartic-Phone, the core gameplay involves a quiz master drawing a word while other players guess it.

<br>

Our project was developed entirely in **_C language_** and designed to run in a **_Linux_** environment.

<br>
<br>
<br>
<br>

### 🖼️ Demonstration

![demonstration1 GIF](/readme/gif/home-setting.gif)  
On the **home screen**, you can access the audio settings by clicking the **setting** button.  
The audio includes background music and sound effects for correct and incorrect answers.  
`(Audio files are located in /Gangtic-Phone/assets/audio.)`

<br>
<br>

![demonstration2 GIF](/readme/gif/home-waiting.gif)  
By clicking the **Create** button, you can create a room, and the **X** button allows you to delete it.  
Clicking on a room lets you enter it, but the game requires 4 players to proceed, so you'll need to wait until the room is full.  
Rooms with other players **_cannot be deleted_**, and rooms with already 4 players are **_not accessible_**.

<br>
<br>

![demonstration3 GIF](/readme/gif/enter-game.gif)  
Once **4 players** are in the room, it is changed to the **game screen**.  
The player who entered the room first becomes the initial quiz master(painter).  
Afterward, the role of the quiz master is passed to the player who answers correctly.

<br>
<br>

![demonstration4 GIF](/readme/gif/game.gif)  
The quiz master draws the answer on the sketchpad.  
This drawing is **visible to all players**, and they can submit their answers using the text-input field.  
If a player answers correctly, they take over as the next quiz master.  
If the answer is incorrect, a message indicating the wrong answer will appear.  
When one player answers correctly, the rest of the players are notified with a corresponding message.

<br>
<br>

![demonstration5 GIF](/readme/gif/game-result.gif)  
The game consists of a total of **5 rounds**.  
After the final round, it will be changed to the **result screen**, where players can view the result of the completed game.  
`(Result files are stored in /Gangtic-Phone/running_game/result_n.txt.)`  
Clicking the **Home** button returns players to the main screen, and the **Exit** button closes the program.

<br>
<br>
<br>
<br>

## 💻 How to use

```bash
git clone https://github.com/kangeunsong/Gangtic-Phone
cd repository
```

Make sure that you're in the right repository.
<br>

```bash
make
./server
./client nickname
```

**You must edit Makefile.**  
Do "make" first to make .exe files. Then, execute 1. server and 2. client(with your nickname).
