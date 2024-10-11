-- 创建测试任务信息表
CREATE TABLE test_info (
    test_id VARCHAR(50) PRIMARY KEY,
    test_name VARCHAR(50) NOT NULL,
    start_date DATE NOT NULL,
    end_date DATE NOT NULL
);

-- 创建测试人员信息表
CREATE TABLE test_user_info (
    user_id VARCHAR(50) PRIMARY KEY,
    rel_test_id VARCHAR(50),
    user_name VARCHAR(50) NOT NULL,
    employment_age INTEGER,
    FOREIGN KEY (rel_test_id) REFERENCES test_info(test_id)
);

-- 创建测试缺陷明细信息表
CREATE TABLE test_bug_detail (
    bug_id VARCHAR(50) PRIMARY KEY,
    rel_test_id VARCHAR(50),
    rel_user_id VARCHAR(50),
    bug_level VARCHAR(50) CHECK (bug_level IN ('严重', '一般', '建议')),
    FOREIGN KEY (rel_test_id) REFERENCES test_info(test_id),
    FOREIGN KEY (rel_user_id) REFERENCES test_user_info(user_id)
);

-- 插入数据到 test_info 表
INSERT INTO test_info (test_id, test_name, start_date, end_date) VALUES
('T001', '功能测试', '2022-02-01', '2022-02-10'),
('T002', '性能测试', '2022-03-05', '2022-03-15'),
('T003', '安全测试', '2022-04-01', '2022-04-10'),
('T004', '回归测试', '2022-02-15', '2022-02-20');

-- 插入数据到 test_user_info 表
INSERT INTO test_user_info (user_id, rel_test_id, user_name, employment_age) VALUES
('U001', 'T001', '张三', 28),
('U002', 'T001', '李四', 32),
('U003', 'T001', '王五', 30),
('U004', 'T001', '赵六', 26),
('U005', 'T001', '钱七', 29),  -- 增加测试人员
('U006', 'T002', '孙八', 31),
('U007', 'T002', '刘九', 27),
('U008', 'T002', '周十', 34),
('U009', 'T003', '吴十一', 29),
('U010', 'T004', '郑十二', 33);

-- 插入数据到 test_bug_detail 表
INSERT INTO test_bug_detail (bug_id, rel_test_id, rel_user_id, bug_level) VALUES
('B001', 'T001', 'U001', '严重'),
('B002', 'T001', 'U002', '一般'),
('B003', 'T001', 'U003', '建议'),
('B004', 'T001', 'U004', '严重'),
('B005', 'T001', 'U005', '一般'),
('B006', 'T001', 'U001', '一般'),
('B007', 'T001', 'U002', '严重'),
('B008', 'T001', 'U003', '建议'),
('B009', 'T001', 'U004', '严重'),
('B010', 'T001', 'U005', '一般'),
('B011', 'T002', 'U006', '一般'),
('B012', 'T002', 'U007', '严重'),
('B013', 'T002', 'U008', '严重'),
('B014', 'T003', 'U009', '严重'),
('B015', 'T004', 'U010', '建议'),
('B016', 'T004', 'U010', '严重');
