package actions

import (
	"net/http"
	"sanctify/sanctify-api-service/model"

	"github.com/gin-gonic/gin"
)

func GetGameserverAddress(c *gin.Context) {
	gameserver := &model.GameServer{
		Hostname:  "localhost",
		WsPort:    9001,
		GameToken: "test-token",
	}

	resp := &model.GetGameServerResponse{
		GameServer: gameserver,
	}

	c.JSON(http.StatusOK, resp)
}
