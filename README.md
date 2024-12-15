# Gangtic-Phone: Drawing Quiz Game using TCP Cocket

&nbsp;

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

&nbsp;
&nbsp;
&nbsp;
&nbsp;

### 🎨 소개

이 프로젝트에 대해 소개해주세요.

&nbsp;
&nbsp;
&nbsp;
&nbsp;

### 🖼️ Demonstration

![demonstration1 GIF](/readme/gif/home-setting.gif)  
On the home screen, you can access the audio settings by clicking the Settings button.  
The audio includes background music and sound effects for correct and incorrect answers.  
(Audio files are located in /Gangtic-Phone/assets/audio).

<br>

![demonstration2 GIF](/readme/gif/home-waiting.gif)  
By clicking the Create button, you can create a room, and the X button allows you to delete it.  
Clicking on a room lets you enter it, but the game requires 4 players to proceed, so you'll need to wait until the room is full.  
Rooms with other players cannot be deleted, and rooms with already 4 players are not accessible.

<br>

![demonstration3 GIF](/readme/gif/enter-game.gif)  
Once 4 players are in the room, it is changed to the game screen.  
The player who entered the room first becomes the initial quiz master(painter). Afterward, the role of the quiz master is passed to the player who answers correctly.

<br>

![demonstration4 GIF](/readme/gif/game.gif)  
The quiz master draws the answer on the sketchpad.  
This drawing is visible to all players, and they can submit their answers using the text-input field.  
If a player answers correctly, they take over as the next quiz master. If the answer is incorrect, a message indicating the wrong answer will appear.  
When one player answers correctly, the rest of the players are notified with a corresponding message.

<br>

![demonstration5 GIF](/readme/gif/game-result.gif)  
The game consists of a total of 5 rounds.  
After the final round, it will be changed to the results screen, where players can view the result of the completed game.  
(Result files are stored in /Gangtic-Phone/running_game/result_n.txt.)  
Clicking the Home button returns players to the main screen, and the Exit button closes the program.

&nbsp;
&nbsp;
&nbsp;
&nbsp;

## 💻 How to use

```bash
git clone https://github.com/kangeunsong/Gangtic-Phone
cd repository
```

Make sure that you're in right repository.
&nbsp;

```bash
make
./server
./client nickname
```

Do "make" first to make .exe files. Then, execute 1. server and 2. client(with your nickname).
