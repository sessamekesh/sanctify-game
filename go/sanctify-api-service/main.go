package main

import (
	"sanctify/sanctify-api-service/actions"

	"github.com/gin-gonic/gin"
)

func main() {
	router := gin.Default()
	router.GET("/api/gameserver", actions.GetGameserverAddress)

	router.Run("localhost:8080")
}
