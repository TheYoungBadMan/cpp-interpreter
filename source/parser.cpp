#include <string>
#include <vector>
#include <memory> 

#include "parser.hpp"

Parser::Parser(
	const std::vector<Token>& tokens
	) : tokens(tokens), offset(0) {}


std::shared_ptr<TranslationUnit> Parser::parse() {
	std::vector<std::shared_ptr<Declaration>> declarations;
	while (!match_token(Token::END)) {
		declarations.push_back(parse_declaration());
	}
	return std::make_shared<TranslationUnit>(declarations);
}

std::shared_ptr<Declaration> Parser::parse_declaration() {
	if (match_pattern(Token::TYPE, Token::IDENTIFIER, Token::LPAREN) || match_pattern(Token::TYPE, Token::MULTIPLY, Token::IDENTIFIER, Token::LPAREN)) {
		return parse_function_declaration();
	} else if (match_pattern(Token::TYPE, Token::IDENTIFIER) || match_pattern(Token::TYPE, Token::MULTIPLY, Token::IDENTIFIER)) {
		return parse_var_declaration();
	} else {
		throw std::runtime_error("Unexpected token " + tokens[offset].value);
	}
}

std::shared_ptr<FuncDeclaration> Parser::parse_function_declaration() {
	auto type = extract_token(Token::TYPE);
	auto declarator = parse_declarator();
	extract_token(Token::LPAREN);

	std::vector<std::shared_ptr<ParameterDeclaration>> args;
	if (!match_token(Token::RPAREN)) {
		while (true) {
			args.push_back(parse_parameter_declaration());
			if (tokens[offset].value == ",") {
				++offset;
			} else if (tokens[offset].value == ")") {
				++offset;
				break;
			} else {
				throw std::runtime_error("Missing closing parenthesis");
			}
		}
	}
	std::shared_ptr<CompoundStatement> body;
	if (match_token(Token::LBRACE)) {
		body = parse_compound_statement();
	} else if (!match_token(Token::SEMICOLON)) {
		throw std::runtime_error("Unexpected token");
	}
	return std::make_shared<FuncDeclaration>(type, declarator, args, body);
}

std::shared_ptr<ParameterDeclaration> Parser::parse_parameter_declaration() {
	return std::make_shared<ParameterDeclaration>(extract_token(Token::TYPE), parse_init_declarator());
}

std::shared_ptr<VarDeclaration> Parser::parse_var_declaration() {
	auto type = extract_token(Token::TYPE);
	std::vector<std::shared_ptr<Declaration::InitDeclarator>> declarator_list;
    while (true) {
        declarator_list.push_back(parse_init_declarator());
        if (match_token(Token::COMMA)) {
            continue;
        } else if (match_token(Token::SEMICOLON)) {
            break;
        } else {
            throw std::runtime_error("Unexpected token " + tokens[offset].value);
        }
    }
    return std::make_shared<VarDeclaration>(type, declarator_list);
}

std::shared_ptr<Declaration::InitDeclarator> Parser::parse_init_declarator() {
	auto declarator = parse_declarator();
	std::shared_ptr<Expression> initializer;
	if (match_token(Token::ASSIGNMENT)) {
		initializer = parse_expression();
	}
	return std::make_shared<Declaration::InitDeclarator>(declarator, initializer);	
}

std::shared_ptr<Declaration::Declarator> Parser::parse_declarator() {
	if (match_pattern(Token::MULTIPLY, Token::IDENTIFIER)) {
		extract_token(Token::MULTIPLY);
		return std::make_shared<Declaration::PtrDeclarator>(extract_token(Token::IDENTIFIER));
	} else if (match_pattern(Token::IDENTIFIER)) {
		return std::make_shared<Declaration::NoPtrDeclarator>(extract_token(Token::IDENTIFIER));
	} else {
		throw std::runtime_error("Unexpected token " + tokens[offset].value);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Statement> Parser::parse_statement() {
	if (match_token(Token::LBRACE)) {
		return parse_compound_statement();
	} else if (match_token(Token::IF)) {
		return parse_conditional_statement();
	} else if (check_token(Token::WHILE, Token::FOR, Token::REPEAT)) {
		return parse_loop_statement();
	} else if (check_token(Token::RETURN, Token::BREAK, Token::CONTINUE)) {
		return parse_jump_statement();
	} else if (check_token(Token::TYPE)) {
		return parse_declaration_statement();
	} else {
		return parse_expression_statement();
	}
}

std::shared_ptr<CompoundStatement> Parser::parse_compound_statement() {
	std::vector<std::shared_ptr<Statement>> statements;
	while (!match_token(Token::RBRACE)) {
		statements.push_back(parse_statement());
	}
	return std::make_shared<CompoundStatement>(statements);
}

std::shared_ptr<ConditionalStatement> Parser::parse_conditional_statement() {
	extract_token(Token::LPAREN);
	auto if_condition = parse_expression();
	extract_token(Token::RPAREN);
	auto if_statement = parse_statement();
	auto if_branch = std::make_pair(if_condition, if_statement);
	std::vector<std::pair<std::shared_ptr<Expression>, std::shared_ptr<Statement>>> elif_branches;
	while (match_token(Token::ELIF)) {
		extract_token(Token::LPAREN);
		auto elif_condition = parse_expression();
		extract_token(Token::RPAREN);
		auto elif_statement = parse_statement();
		auto elif_branch = std::make_pair(elif_condition, elif_statement);
		elif_branches.push_back(elif_branch);
	}
	std::shared_ptr<Statement> else_branch;
	if (match_token(Token::ELSE)) {
		else_branch = parse_statement();
	}
	return std::make_shared<ConditionalStatement>(if_branch, elif_branches, else_branch);
}

std::shared_ptr<LoopStatement> Parser::parse_loop_statement() {
	if (match_token(Token::WHILE)) {
		return parse_while_statement();
	} else if (match_token(Token::FOR)) {
		return parse_for_statement();
	} else {
		extract_token(Token::REPEAT);
		return parse_repeat_statement();
	}
}

std::shared_ptr<WhileStatement> Parser::parse_while_statement() {
	extract_token(Token::LPAREN);
	auto condition = parse_expression();
	extract_token(Token::RPAREN);
	auto statement = parse_statement();
	return std::make_shared<WhileStatement>(condition, statement);
}

std::shared_ptr<RepeatStatement> Parser::parse_repeat_statement() {
	return std::make_shared<RepeatStatement>(parse_statement());
}

std::shared_ptr<ForStatement> Parser::parse_for_statement() {
	return std::make_shared<ForStatement>();
}

std::shared_ptr<JumpStatement> Parser::parse_jump_statement() {
	if (match_token(Token::BREAK)) {
		return parse_break_statement();
	} else if (match_token(Token::CONTINUE)) {
		return parse_continue_statement();
	} else {
		extract_token(Token::RETURN);
		return parse_return_statement();
	}
}

std::shared_ptr<BreakStatement> Parser::parse_break_statement() {
	extract_token(Token::SEMICOLON);
	return std::make_shared<BreakStatement>();
}

std::shared_ptr<ContinueStatement> Parser::parse_continue_statement() {
	extract_token(Token::SEMICOLON);
	return std::make_shared<ContinueStatement>();
}

std::shared_ptr<ReturnStatement> Parser::parse_return_statement() {
	auto expression = parse_expression();
	extract_token(Token::SEMICOLON);
	return std::make_shared<ReturnStatement>(expression);
}

std::shared_ptr<DeclarationStatement> Parser::parse_declaration_statement() {
	auto declaration = parse_var_declaration();
	return std::make_shared<DeclarationStatement>(declaration);
}

std::shared_ptr<ExpressionStatement> Parser::parse_expression_statement() {
	auto expression = parse_expression();
	extract_token(Token::SEMICOLON);
	return std::make_shared<ExpressionStatement>(expression);
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////////

std::shared_ptr<Expression> Parser::parse_expression() {
	return parse_binary_expression(0);
}

std::shared_ptr<BinaryExpression> Parser::parse_binary_expression(int min_precedence) {
	std::shared_ptr<BinaryExpression> lhs = parse_unary_expression();
	for (auto op = tokens[offset].value; operator_precedences.contains(op) && operator_precedences.at(op) >= min_precedence; op = tokens[offset].value) {
		++offset;
		lhs = std::make_shared<BinaryOperation>(op, lhs, parse_binary_expression(operator_precedences.at(op)));
	}
	return lhs;
}

std::shared_ptr<UnaryExpression> Parser::parse_unary_expression() {
	if (auto op = tokens[offset].value; unary_operators.contains(op)) {
		++offset;
		return std::make_shared<PrefixExpression>(op, parse_unary_expression());
	}
	return parse_postfix_expression();
}

std::shared_ptr<PostfixExpression> Parser::parse_postfix_expression() {
	std::shared_ptr<PostfixExpression> base = parse_primary_expression();
	while (true) {
		if (match_token(Token::INCREMENT)) {
			base = std::make_shared<PostfixIncrementExpression>(base);
		} else if (match_token(Token::DECREMENT)) {
			base = std::make_shared<PostfixDecrementExpression>(base);
		} else if (match_token(Token::LPAREN)) {
			base = std::make_shared<FunctionCallExpression>(base, parse_function_call_expression());
		} else if (match_token(Token::LBRACKET)) {
			base = std::make_shared<SubscriptExpression>(base, parse_subscript_expression());
		} else {
			break;
		}
	}
	return base;
}

std::vector<std::shared_ptr<Expression>> Parser::parse_function_call_expression() {
	std::vector<std::shared_ptr<Expression>> args;
	if (!match_token(Token::RPAREN)) {
		while (true) {
			args.push_back(parse_expression());
			if (!match_token(Token::COMMA)) {
				break;
			}
			extract_token(Token::COMMA);
		}
	}
	extract_token(Token::RPAREN);
	return args;
}

std::shared_ptr<Expression> Parser::parse_subscript_expression() {
	auto index = parse_expression();
	extract_token(Token::RBRACKET);
	return index;
}

std::shared_ptr<PrimaryExpression> Parser::parse_primary_expression() {
	if (check_token(Token::INTEGER_LITERAL)) {
		return std::make_shared<IntLiteral>(extract_token(Token::INTEGER_LITERAL));
	} else if (check_token(Token::FLOAT_LITERAL)) {
		return std::make_shared<FloatLiteral>(extract_token(Token::FLOAT_LITERAL));
	} else if (check_token(Token::CHAR_LITERAL)) {
		return std::make_shared<CharLiteral>(extract_token(Token::CHAR_LITERAL));
	} else if (check_token(Token::STRING_LITERAL)) {
		return std::make_shared<StringLiteral>(extract_token(Token::STRING_LITERAL));
	} else if (check_token(Token::BOOL_LITERAL)) {
		return std::make_shared<BoolLiteral>(extract_token(Token::BOOL_LITERAL));
	} else if (check_token(Token::IDENTIFIER)) {
		return std::make_shared<IdentifierExpression>(extract_token(Token::IDENTIFIER));
	} else if (match_token(Token::LPAREN)) {
		return parse_parenthesized_expression();
	} else {
		throw std::runtime_error("Unexpected token " + tokens[offset].value);
	}
}

std::shared_ptr<ParenthesizedExpression> Parser::parse_parenthesized_expression() {
	auto expression = parse_expression();
	extract_token(Token::RPAREN);
	return std::make_shared<ParenthesizedExpression>(expression);
}

///////////////////////////////////////////////////////////////////////////////////

template<typename... Args>
bool Parser::check_token(const Args&... expected) {
	return ((tokens[offset].type == expected) || ...);
}

template<typename... Args>
bool Parser::match_token(const Args&... expected) {
	bool match_found = check_token(expected...);
	if (match_found) {
		++offset;
	}
	return match_found;
}

template<typename... Args>
std::string Parser::extract_token(const Args&... expected) {
	if (!((tokens[offset].type == expected) || ...)) {
		throw std::runtime_error("Unexpected token " + tokens[offset].value);
	}
	return tokens[offset++].value;
}

template<typename... Args>
bool Parser::match_pattern(const Args&... expected) {
	std::size_t i = offset;
	return ((tokens[i++].type == expected) && ...);
}

//////////////////////////////////////////////////////////

const std::unordered_map<std::string, int> Parser::operator_precedences = {
	{"=", 0}, {"+=", 0}, {"-=", 0}, {"*=", 0}, {"/=", 0}, {"%=", 0}, {"**=", 0},
	{"||", 1},
	{"&&", 2},
	{"==", 3}, {"!=", 3},
	{"<", 4}, {"<=", 4}, {">", 4}, {">=", 4},
	{"+", 5}, {"-", 5},
	{"*", 6}, {"/", 6}, {"%", 6},
	{"^", 7}
};

const std::unordered_set<std::string> Parser::unary_operators = {
	"+", "-", "&", "*", "!", "++", "--"
};