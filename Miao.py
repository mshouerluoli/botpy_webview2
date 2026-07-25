# 标准库
from ast import Pass
import asyncio, os, time, json, re
import urllib.request, urllib.parse
from datetime import datetime, timedelta
from typing import List, Dict, Any

# 第三方库
import botpy
from botpy import logging, BotAPI
from botpy.message import C2CMessage, GroupMessage, Message
from botpy.ext.cog_yaml import read

from botpy.types.message import Ark as MessageArk, ArkKv as MessageArkKv
from botpy.types.message import Message as MessagePayload,MarkdownPayload

from botpy.interaction import Interaction
from apscheduler.schedulers.asyncio import AsyncIOScheduler
from apscheduler.triggers.cron import CronTrigger
import aiohttp, aiofiles
from pathlib import Path
from PIL import Image
import numpy as np

# 自定义模块
import kurobbs, mihoyo, wangyi,verifykey,oiAPI,hoyolab
import genshin_scraper
import siliconflow

test_config = read(os.path.join(os.path.dirname(__file__), "config.yaml"))

_log = logging.get_logger()



class MyClient(botpy.Client):
    # 机器人就绪监听
    async def on_ready(self):
        _log.info(f"[Miao]「{self.robot.name}」 on_ready!")



    async def _handle_common_commands(self, message,message_isgroup:bool):
       print(f"_handle_common_commands: {isgroup}")

    # 私聊消息监听
    async def on_c2c_message_create(self, message: C2CMessage):
        await self._handle_common_commands(message,False)

    # 群聊消息监听 
    async def on_group_at_message_create(self, message: GroupMessage):
        await self._handle_common_commands(message,True)


if __name__ == "__main__":
    # 通过kwargs，设置需要监听的事件通道
    """
    public_messages 群/C2C公域消息事件
    public_guild_messages	公域消息事件
    guild_messages	消息事件 (仅 私域 机器人能够设置此 intents)
    direct_message	私信事件
    guild_message_reactions	消息相关互动事件
    guilds	频道事件
    guild_members	频道成员事件
    interaction	互动事件
    message_audit	消息审核事件
    forums	论坛事件 (仅 私域 机器人能够设置此 intents)
    audio_action	音频事件
    """
    intents = botpy.Intents(public_messages=True,direct_message=True)

    #intents = botpy.Intents().all()
    client = MyClient(intents=intents)
    client.run(appid=test_config["appid"], secret=test_config["secret"])