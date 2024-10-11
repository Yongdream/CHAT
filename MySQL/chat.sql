/*
 Navicat Premium Dump SQL

 Source Server         : yang
 Source Server Type    : MySQL
 Source Server Version : 80039 (8.0.39-0ubuntu0.22.04.1)
 Source Host           : 49.235.166.80:3306
 Source Schema         : chat

 Target Server Type    : MySQL
 Target Server Version : 80039 (8.0.39-0ubuntu0.22.04.1)
 File Encoding         : 65001

 Date: 14/09/2024 14:29:48
*/

SET NAMES utf8mb4;
SET FOREIGN_KEY_CHECKS = 0;

-- ----------------------------
-- Table structure for allgroup
-- ----------------------------
DROP TABLE IF EXISTS `allgroup`;
CREATE TABLE `allgroup`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `groupname` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `groupdesc` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NULL DEFAULT '',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of allgroup
-- ----------------------------

-- ----------------------------
-- Table structure for friend
-- ----------------------------
DROP TABLE IF EXISTS `friend`;
CREATE TABLE `friend`  (
  `userid` int NOT NULL,
  `friendid` int NOT NULL,
  PRIMARY KEY (`friendid`, `userid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of friend
-- ----------------------------
INSERT INTO `friend` VALUES (2, 1);
INSERT INTO `friend` VALUES (1, 2);

-- ----------------------------
-- Table structure for groupuser
-- ----------------------------
DROP TABLE IF EXISTS `groupuser`;
CREATE TABLE `groupuser`  (
  `groupid` int NOT NULL,
  `userid` int NOT NULL,
  `grouprole` enum('creator','normal') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'normal',
  PRIMARY KEY (`groupid`, `userid`) USING BTREE
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of groupuser
-- ----------------------------

-- ----------------------------
-- Table structure for offlinemessage
-- ----------------------------
DROP TABLE IF EXISTS `offlinemessage`;
CREATE TABLE `offlinemessage`  (
  `userid` int NOT NULL,
  `message` varchar(255) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL
) ENGINE = InnoDB CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of offlinemessage
-- ----------------------------
INSERT INTO `offlinemessage` VALUES (3, '{\"id\":2,\"msg\":\"Hello,Hello.\",\"msgid\":5,\"name\":\"yz\",\"to\":3}');

-- ----------------------------
-- Table structure for user
-- ----------------------------
DROP TABLE IF EXISTS `user`;
CREATE TABLE `user`  (
  `id` int NOT NULL AUTO_INCREMENT,
  `name` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `password` varchar(50) CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL,
  `state` enum('online','offline') CHARACTER SET utf8mb4 COLLATE utf8mb4_0900_ai_ci NOT NULL DEFAULT 'offline',
  PRIMARY KEY (`id`) USING BTREE
) ENGINE = InnoDB AUTO_INCREMENT = 5 CHARACTER SET = utf8mb4 COLLATE = utf8mb4_0900_ai_ci ROW_FORMAT = Dynamic;

-- ----------------------------
-- Records of user
-- ----------------------------
INSERT INTO `user` VALUES (1, 'demo', '123456', 'online');
INSERT INTO `user` VALUES (2, 'yz', '123456', 'offline');
INSERT INTO `user` VALUES (3, 'demo2', '123456', 'offline');
INSERT INTO `user` VALUES (4, 'sc', '123456', 'offline');

SET FOREIGN_KEY_CHECKS = 1;
