import pygame
import ctypes
import sysv_ipc

# 遊戲設定
WIDTH, HEIGHT = 1000, 600
FPS = 60
SHM_KEY = 0x4D2
DIGLETT_NUM = 5
MAX_PLAYER = 3  # 支援多玩家

# 記憶體結構
class ShmStruct(ctypes.Structure):
    _fields_ = [
        ("diglett", ctypes.c_int * DIGLETT_NUM),
        ("point", ctypes.c_int * MAX_PLAYER),
        ("game_time", ctypes.c_int),
        ("playerID", ctypes.c_int),
    ]

# 初始化 Pygame
pygame.display.init()
pygame.font.init()
pygame.display.set_caption("Whack a Mole")
screen = pygame.display.set_mode((WIDTH, HEIGHT))
clock = pygame.time.Clock()

# 載入背景圖片
background_image = pygame.image.load("test2.png").convert()
background_image = pygame.transform.scale(background_image, (WIDTH, HEIGHT))

# 載入地洞圖片
hole_image = "hole.png"  # 地洞圖片
hole_surface = pygame.image.load(hole_image).convert_alpha()
hole_surface = pygame.transform.scale(hole_surface, (100, 100))  # 縮放為地洞大小

# 載入 Game Over 圖片
gameover_image = pygame.image.load("gameover.png").convert()
gameover_image = pygame.transform.scale(gameover_image, (WIDTH, HEIGHT))  # 縮放至螢幕大小

# 地鼠類別
class Mole(pygame.sprite.Sprite):
    def __init__(self, x, y):
        super().__init__()
        self.image = pygame.Surface((150, 150), pygame.SRCALPHA)
        self.rect = self.image.get_rect(center=(x, y))

    def set_image(self, image_path, scale=None):
        """設定地鼠的圖片，可選擇縮放大小"""
        self.image = pygame.image.load(image_path).convert_alpha()
        if scale:
            self.image = pygame.transform.scale(self.image, scale)
        else:
            self.image = pygame.transform.scale(self.image, (150, 150))
        self.rect = self.image.get_rect(center=self.rect.center)

    def reset_image(self):
        """重置地鼠圖片為透明"""
        self.image = pygame.Surface((150, 150), pygame.SRCALPHA)
        self.rect = self.image.get_rect(center=self.rect.center)

# 地鼠和地洞位置
positions = [
    (WIDTH / 2 - 400, HEIGHT / 2),  # 最左邊
    (WIDTH / 2 - 200, HEIGHT / 2),  # 中間偏左
    (WIDTH / 2, HEIGHT / 2),        # 正中間
    (WIDTH / 2 + 200, HEIGHT / 2),  # 中間偏右
    (WIDTH / 2 + 400, HEIGHT / 2),  # 最右邊
]

# 地洞位置
hole_offsets = [
    (0, 30),
    (0, 30),
    (0, 30),
    (0, 30),
    (0, 30),
]

# 創建地鼠物件
moles = [Mole(*pos) for pos in positions]
all_sprites = pygame.sprite.Group(moles)

# 載入圖片
red_image = "diglett.png"  # 紅色地鼠圖片
yellow_image = "Ex_diglett.png"  # 黃色地鼠圖片
green_image = "Bomb.png"  # 綠色地鼠圖片

# 初始化共享記憶體
memory = sysv_ipc.SharedMemory(SHM_KEY)

# 遊戲主迴圈
running = True
while running:
    clock.tick(FPS)

    # 從共享記憶體讀取數據
    data = memory.read(ctypes.sizeof(ShmStruct))
    shm_data = ShmStruct.from_buffer_copy(data)

    # 更新地鼠狀態
    for i, mole in enumerate(moles):
        value = shm_data.diglett[i]
        if value == -1:
            mole.set_image(green_image, scale=(200, 200))  # 放大炸彈圖片
        elif value == 1:
            mole.set_image(red_image)  # 紅色地鼠
        elif value == 2:
            mole.set_image(yellow_image)  # 黃色地鼠
        else:
            mole.reset_image()  # 重置為透明

    # 繪製畫面
    screen.blit(background_image, (0, 0))  # 繪製背景圖片

    # 繪製地洞圖片（根據地鼠位置和偏移量）
    for mole, offset in zip(moles, hole_offsets):
        # 計算地洞的位置
        hole_rect = hole_surface.get_rect(center=(mole.rect.centerx + offset[0], mole.rect.centery + offset[1]))
        screen.blit(hole_surface, hole_rect)

    # 繪製地鼠圖片
    for mole in moles:
        screen.blit(mole.image, mole.rect)  # 在地洞上方繪製地鼠

    # 顯示分數和剩餘時間
    font = pygame.font.SysFont(None, 36)
    for player_id in range(MAX_PLAYER):
        score_text = font.render(f"Player {player_id + 1} Score: {shm_data.point[player_id]}", True, (0, 0, 0))
        screen.blit(score_text, (10, 10 + player_id * 40))  # 不同玩家的分數顯示在不同高度
    time_text = font.render(f"Time Left: {shm_data.game_time}s", True, (0, 0, 0))
    player_text = font.render(f"Player: {shm_data.playerID}", True, (0, 0, 0))
    screen.blit(time_text, (400, 20))  # 繪製剩餘時間
    screen.blit(player_text, (400, 50))  # 繪製玩家 ID

    # 判斷遊戲是否結束
    if shm_data.game_time <= 0:
        # 先清空畫面，繪製 Game Over 圖片
        screen.blit(gameover_image, (0, 0))  # 繪製 Game Over 圖片

        # 動畫顯示多個玩家分數
        font = pygame.font.SysFont(None, 60)
        final_scores = shm_data.point  # 各玩家的最終分數
        current_scores = [0] * MAX_PLAYER  # 初始化每個玩家的分數為 0
        increments = [max(1, abs(score) // 100) for score in final_scores]  # 根據分數絕對值設置步伐
        delay = 80  # 每次更新的延遲時間（毫秒）

        # 逐步顯示分數的動畫
        animation_running = True
        while animation_running:
            animation_running = False  # 預設動畫完成
            screen.blit(gameover_image, (0, 0))  # 繪製 Game Over 圖片

            x_name = WIDTH // 2 - 240  # 固定玩家名稱的 X 坐標
            x_score = WIDTH // 2 - 30 # 固定分數的 X 坐標

            # 事件處理
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    animation_running = False
                    running = False

            for player_id in range(MAX_PLAYER):
                if final_scores[player_id] >= 0:
                    # 遞增邏輯
                    if current_scores[player_id] < final_scores[player_id]:
                        animation_running = True
                        current_scores[player_id] += increments[player_id]
                        if current_scores[player_id] > final_scores[player_id]:
                            current_scores[player_id] = final_scores[player_id]
                else:
                    # 遞減邏輯
                    if current_scores[player_id] > final_scores[player_id]:
                        animation_running = True
                        current_scores[player_id] -= increments[player_id]
                        if current_scores[player_id] < final_scores[player_id]:
                            current_scores[player_id] = final_scores[player_id]

                # 繪製玩家名稱
                name_text = font.render(f"Player {player_id + 1}", True, (255, 255, 255))
                name_rect = name_text.get_rect(midleft=(x_name, HEIGHT // 2 - 30 + player_id * 60))
                screen.blit(name_text, name_rect)

                # 動態處理正負號顯示
                formatted_score = str(current_scores[player_id])
                score_text = font.render(f"Final Score: {formatted_score}", True, (255, 255, 255))
                score_rect = score_text.get_rect(midleft=(x_score, HEIGHT // 2 - 30 + player_id * 60))
                screen.blit(score_text, score_rect)

            # 更新畫面
            pygame.display.flip()

            # 控制動畫更新頻率
            clock.tick(1000 // delay)

        # 停留額外時間以顯示完整分數
        pygame.time.wait(40000)

        # 離開主迴圈
        running = False
    else:
        # 如果還沒結束，就一般更新畫面
        pygame.display.flip()

pygame.quit()
